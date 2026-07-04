/**
  ******************************************************************************
  * @file    nnc6521_drv.c
  * @brief   NNC6521 高级驱动层，基于 ens1p4.c 适配双芯片软件 SPI 架构。
  *          所有 SPI 调用通过 nnc6521_spi_xxx 系列函数路由，支持 chip_id 选择。
  *          包含波形发生器配置、导联脱落检测 (LOD)、短路检测 (SCD)、
  *          幅度调制 (AM)、电流校准等核心功能。
  ******************************************************************************
  */

#include "nnc6521.h"

/* ============================================================================
 *  内部函数声明（电流校准相关）
 * ===========================================================================*/

/**
 * @brief 生成缩放后的波形电流数组
 *
 * 将归一化浮点波形数组乘以目标幅度值，并限制最大值为 4095 (12-bit DAC 满量程)。
 * 四舍五入取整后存入 16-bit 输出数组。
 *
 * @param[out] output      输出的 16-bit 电流值数组
 * @param[in]  u8_PointNum 波形采样点数
 * @param[in]  base_wave   归一化浮点波形数据（范围 0.0~1.0）
 * @param[in]  amplitude   目标幅度值（由 Current_Output 计算得到）
 */
static void generate_scaled_wave(uint16_t *output, uint8_t u8_PointNum,
                                 float *base_wave, float amplitude);

/**
 * @brief 将 16-bit 电流数组转换为 8-bit 数组
 *
 * 找到数组中的最大值，计算所需位数，确定右移量（scale_up），
 * 然后将所有数据右移后截断为 8-bit。用于通过 8-bit SPI 接口传输 12-bit 数据。
 *
 * @param[out] dst        输出的 8-bit 数组
 * @param[out] bits_shift 实际右移位数（用于后续恢复 scale_up）
 * @param[in]  src        输入的 16-bit 数组
 * @param[in]  length     数组长度
 */
static void convert_16bit_to_8bit(uint8_t *dst, uint8_t *bits_shift,
                                  uint16_t *src, int length);

/**
 * @brief 计算一个 16-bit 值实际使用的位数
 *
 * @param[in] value 输入数值
 * @return 使用的位数（例如 0xFF 返回 8，0x100 返回 9）
 */
static uint8_t bits_used(uint16_t value);

/**
 * @brief 判断电流值是否在指定范围内
 *
 * @param[in] current 待检测电流值
 * @param[in] Imin    范围下限
 * @param[in] Imax    范围上限
 * @retval 1 电流值在 [Imin, Imax] 范围内
 * @retval 0 电流值超出范围
 */
static uint8_t CurIsInSide(uint16_t current, uint16_t Imin, uint16_t Imax);

/**
 * @brief 计算电流值到范围边界的距离
 *
 * 若电流值在范围内则返回 0，否则返回到最近边界的距离。
 *
 * @param[in] current 待检测电流值
 * @param[in] Imin    范围下限
 * @param[in] Imax    范围上限
 * @return 到范围边界的距离（在范围内返回 0）
 */
static uint16_t GetDif(uint16_t current, uint16_t Imin, uint16_t Imax);

/**
 * @brief 从 OTP 校准数据中查找匹配电流值的 Dref 参考值和分段索引
 *
 * 遍历 OTP 中存储的校准分段数据，每段对应一个 Dref 值。
 * 根据 Dref 计算每段的电流范围 [Imin, Imax]，找到包含目标电流的分段。
 * 若无精确匹配，选择距离最近的分段。
 *
 * @param[in]  chip_id  芯片编号
 * @param[in]  Channel  通道编号
 * @param[in]  current  目标电流值
 * @param[out] aSeg     匹配到的分段索引
 * @return 匹配到的 Dref 参考值
 */
static uint16_t GetDrefIselIseg(uint8_t chip_id, uint8_t aChan,
                                uint16_t current, uint8_t *aSeg);

/**
 * @brief 将目标电流值转换为 12-bit DAC 数值
 *
 * 调用 GetDrefIselIseg 获取校准分段的 Dref 值，然后计算：
 * Ival = (current * 1000) / Dref，结果限制在 0~4096 范围内。
 *
 * @param[in] chip_id  芯片编号
 * @param[in] current  目标电流值（单位：mA）
 * @param[in] channel  通道编号
 * @return 12-bit DAC 数值（0~4096）
 */
static uint32_t Current_Output(uint8_t chip_id, uint32_t current, uint8_t channel);

/**
 * @brief NNC6521 芯片上电初始化
 *
 * 执行完整的上电复位序列：
 * 1. 拉低 CHIP_EN 引脚，等待约 10ms（通过空循环实现）
 * 2. 拉高 CHIP_EN 引脚，再等待约 10ms，芯片完成内部复位
 * 3. 写 PMU_REG_ADDR = 0x00 复位波形发生器
 * 4. 配置 CLK_CTRL_REG_ADDR = 0x00，PCLK 分频系数 = 1（2 MHz）
 * 5. 写 WAVEGEN_GLOBAL_REG_0 = 0x01 使能全局驱动
 *
 * @note CHIP_EN 引脚映射：CHIP_1 = PC5，CHIP_2 = PC7
 * @note 使用空循环延时而非 rt_thread_mdelay，因可能在调度器启动前调用
 *
 * @param[in] chip_id 芯片编号，NNC6521_CHIP_1 或 NNC6521_CHIP_2
 *
 * @see nnc6521_gpio_init()
 */
void nnc6521_init(uint8_t chip_id)
{
    if (chip_id >= NNC6521_NUM_CHIPS) return;

    /* CHIP_EN 上电复位序列：拉低 → 延时 → 拉高 → 延时 → 就绪 */
    /* GPIO 引脚映射与 nnc6521_spi.c 中的引脚表一致，此处直接使用 HAL 调用 */
    switch (chip_id) {
        case NNC6521_CHIP_1:
            /* CHIP_EN = PC5 */
            __HAL_RCC_GPIOC_CLK_ENABLE();
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
            { volatile uint32_t d = 360000; while (d--) __NOP(); }  /* ~5ms @ 72MHz */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
            { volatile uint32_t d = 36000000; while (d--) __NOP(); }  /* ~500ms @ 72MHz */
            break;

        case NNC6521_CHIP_2:
            /* CHIP_EN = PC7 */
            __HAL_RCC_GPIOC_CLK_ENABLE();
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
            { volatile uint32_t d = 360000; while (d--) __NOP(); }  /* ~5ms @ 72MHz */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
            { volatile uint32_t d = 36000000; while (d--) __NOP(); }  /* ~500ms @ 72MHz */
            break;
    }

    /* 复位波形发生器 */
    nnc6521_write_reg(chip_id, PMU_REG_ADDR, 0x00);

    /* 基本时钟和模拟前端配置 */
    nnc6521_write_reg(chip_id, CLK_CTRL_REG_ADDR, 0x00);     /* PCLK 分频 = 1（2 MHz） */
    nnc6521_write_reg(chip_id, WAVEGEN_GLOBAL_REG_0, 0x01);  /* 全局驱动使能 */
}

/**
 * @brief 使能或禁用指定通道的任意波形发生器 (AWG)
 *
 * 读取当前通道的 WG_DRV_CTRL_REG0 寄存器，修改 enable_wavegen 位后写回。
 * 在切换波形前应先禁用 AWG，配置完成后再使能。
 *
 * @param[in] chip_id         芯片编号
 * @param[in] AWG_ChannelNum  通道编号，WAVEFORM_GEN_CH0 或 WAVEFORM_GEN_CH1
 * @param[in] Enable_Disable  1 = 使能，0 = 禁用
 *
 * @see nnc6521_wavegen_config()
 */
void nnc6521_awg_enable_disable(uint8_t chip_id, uint8_t AWG_ChannelNum,
                                uint8_t Enable_Disable)
{
    waveform_TypeDef wf = {0};
    wf.WG_DRV_CTRL_REG0.value = nnc6521_read_wave_reg(chip_id, WG_REG_ADDR(AWG_ChannelNum, WG_DRV_CTRL_REG0_OFFSET));
    wf.WG_DRV_CTRL_REG0.bits.enable_wavegen = Enable_Disable;
    nnc6521_write_wave_reg(chip_id,WG_REG_ADDR(AWG_ChannelNum, WG_DRV_CTRL_REG0_OFFSET),wf.WG_DRV_CTRL_REG0.value);
}

/**
 * @brief 配置并输出预加载波形
 *
 * 使用芯片内置的预加载波形（正弦、三角、脉冲等），无需通过 SPI 传输波形数据。
 * 配置波形参数后调用 nnc6521_wavegen_config() 完成寄存器写入和使能。
 *
 * @param[in] chip_id               芯片编号
 * @param[in] u8_Channel             通道编号
 * @param[in] u8_Waveform            预加载波形类型（WAVEFORM_SINE / WAVEFORM_TRIANGLE / WAVEFORM_PULSE）
 * @param[in] u8_PointNum            每半波采样点数（16 / 32 / 64）
 * @param[in] u8_CI                  驱动电流索引（CI），控制输出电流档位
 * @param[in] u32_Positive_Interval  正半波间隔时钟数（half_wave_clk）
 * @param[in] u32_Negative_Interval  负半波间隔时钟数（通常与正半波相同）
 * @param[in] u32_Silent_Time        静默时间时钟数（24-bit）
 * @param[in] u16_Rest_Time          死区时间时钟数
 *
 * @note 波形自动循环输出（continue_repeat = 1）
 * @note 使用多电极模式（multi_electrode = 1）
 *
 * @see nnc6521_wavegen_config()
 * @see nnc6521_customized_waveform()
 */
void nnc6521_preloaded_waveform(uint8_t chip_id,
                                uint8_t u8_Channel,
                                uint8_t u8_Waveform,
                                uint8_t u8_PointNum,
                                uint8_t u8_CI,
                                uint16_t u32_Positive_Interval,
                                uint16_t u32_Negative_Interval,
                                uint32_t u32_Silent_Time,
                                uint16_t u16_Rest_Time)
{
    waveform_TypeDef wf = {0};
    wf.CHANNEL = u8_Channel;
    wf.WG_DRV_POINT_CONFIG.value = u8_PointNum;
    wf.WG_DRV_CTRL_REG0.bits.symmetric_wave = 0;  /* 非对称波形 */

    wf.WG_DRV_CONFIG_REG0.bits.rest_enable = 1;        /* 死区使能 */
    wf.WG_DRV_CONFIG_REG0.bits.negative_enable = 1;    /* 负半波使能 */
    wf.WG_DRV_CONFIG_REG0.bits.silent_enable = 1;      /* 静默期使能 */
    wf.WG_DRV_CONFIG_REG0.bits.sourceB_enable = 1;     /* Source B 使能 */
    wf.WG_DRV_CONFIG_REG0.bits.multi_electrode = 1;    /* 多电极模式 */
    wf.WG_DRV_CONFIG_REG0.bits.continue_repeat = 1;    /* 连续重复输出 */
    wf.WG_DRIVE_REG_CTRL2.bits.out_pos = u8_CI;        /* 设置驱动电流档位 */
    wf.WG_DRV_SILENT_CLK.value = u32_Silent_Time;
    wf.WG_DRV_HALF_WAVE_CLK_PNT.value = u32_Positive_Interval;
    wf.WG_DRV_NEG_HALF_WAVE_CLK_PNT.value = u32_Negative_Interval;
    wf.WG_DRV_REST_CLK.value = u16_Rest_Time;
    wf.WG_DRV_CTRL_REG0.bits.enable_wavegen = 1;       /* 使能波形发生器 */
    wf.WG_DRV_CTRL_REG0.bits.waveform_select = u8_Waveform;  /* 选择预加载波形 */
    wf.WG_DRV_CTRL_REG0.bits.preload_mode = 0;         /* 非预加载模式 */

    nnc6521_wavegen_config(chip_id, &wf, NULL, 0);
}

/**
 * @brief 配置并输出自定义 SPI 波形
 *
 * 通过 SPI 接口将自定义归一化波形数据传输到芯片 RAM，
 * 内部会自动执行电流校准流程：
 * 1. 调用 Current_Output() 将目标电流转换为 12-bit DAC 数值
 * 2. 调用 generate_scaled_wave() 将归一化数据缩放到 DAC 量程
 * 3. 调用 convert_16bit_to_8bit() 适配 8-bit SPI 传输
 * 4. 逐点写入波形 RAM
 *
 * @param[in] chip_id               芯片编号
 * @param[in] u8_Channel             通道编号
 * @param[in] u8_PointNum            波形采样点数
 * @param[in] f_Normalized_array     归一化浮点波形数据（0.0~1.0）
 * @param[in] u32_Max_current        最大输出电流（单位：mA）
 * @param[in] u32_Positive_Interval  正半波间隔时钟数
 * @param[in] u32_Negative_Interval  负半波间隔时钟数
 * @param[in] u32_Silent_Time        静默时间时钟数
 * @param[in] u16_Rest_Time          死区时间时钟数
 * @param[in] u8_Asymmetric_Symmetric 波形对称性：0 = 非对称，1 = 对称（仅传输半波）
 *
 * @note 对称模式下 point_num 自动减半，硬件自动镜像正半波到负半波
 *
 * @see nnc6521_preloaded_waveform()
 * @see nnc6521_wavegen_config()
 */
void nnc6521_customized_waveform(uint8_t chip_id,
                                 uint8_t u8_Channel,
                                 uint8_t u8_PointNum,
                                 float *f_Normalized_array,
                                 uint32_t u32_Max_current,
                                 uint16_t u32_Positive_Interval,
                                 uint16_t u32_Negative_Interval,
                                 uint32_t u32_Silent_Time,
                                 uint16_t u16_Rest_Time,
                                 uint8_t u8_Asymmetric_Symmetric)
{
    waveform_TypeDef wf = {0};
    wf.CHANNEL = u8_Channel;

    if (u8_Asymmetric_Symmetric == 0) {
        wf.WG_DRV_POINT_CONFIG.value = u8_PointNum;        /* 非对称：使用完整点数 */
        wf.WG_DRV_CTRL_REG0.bits.symmetric_wave = 0;
    } else {
        wf.WG_DRV_POINT_CONFIG.value = u8_PointNum / 2;    /* 对称：仅使用半波点数 */
        wf.WG_DRV_CTRL_REG0.bits.symmetric_wave = 1;
    }

    wf.WG_DRV_CONFIG_REG0.bits.rest_enable = 1;        /* 死区使能 */
    wf.WG_DRV_CONFIG_REG0.bits.negative_enable = 1;    /* 负半波使能 */
    wf.WG_DRV_CONFIG_REG0.bits.silent_enable = 1;      /* 静默期使能 */
    wf.WG_DRV_CONFIG_REG0.bits.sourceB_enable = 1;     /* Source B 使能 */
    wf.WG_DRV_CONFIG_REG0.bits.multi_electrode = 1;    /* 多电极模式 */
    wf.WG_DRV_CONFIG_REG0.bits.continue_repeat = 1;    /* 连续重复输出 */
    wf.WG_DRV_SILENT_CLK.value = u32_Silent_Time;
    wf.WG_DRV_HALF_WAVE_CLK_PNT.value = u32_Positive_Interval;
    wf.WG_DRV_NEG_HALF_WAVE_CLK_PNT.value = u32_Negative_Interval;
    wf.WG_DRV_REST_CLK.value = u16_Rest_Time;
    wf.WG_DRV_CTRL_REG0.bits.enable_wavegen = 1;       /* 使能波形发生器 */
    wf.WG_DRV_CTRL_REG0.bits.waveform_select = WAVEFORM_SPI;  /* 选择 SPI 自定义波形 */
    wf.WG_DRV_CTRL_REG0.bits.preload_mode = 0;

    nnc6521_wavegen_config(chip_id, &wf, f_Normalized_array, u32_Max_current);
}

/**
 * @brief 配置并输出幅度调制 (AM) 波形
 *
 * 实现包络波 × 载波的幅度调制输出：
 * - 包络波（Envelope）：通过 SPI 传输的自定义归一化波形数据，
 *   控制每个包络采样点的幅度
 * - 载波（Carrier）：由 WG_DRV_ALT_LIM 寄存器控制频率，
 *   通常为 4 kHz（250 个 PCLK 时钟的半周期）
 * - 包络更新间隔由 WG_DRV_HALF_WAVE_CLK_PNT 控制
 *
 * 交替模式（alternating_pos = 1）下，载波在正负方向交替输出，
 * 形成真正的交流调制效果。
 *
 * @param[in] chip_id                      芯片编号
 * @param[in] u8_Channel                    通道编号
 * @param[in] u8_PointNum                   包络波采样点数
 * @param[in] f_Normalized_Envelope_array   归一化包络波数据（0.0~1.0）
 * @param[in] u32_Max_current               最大输出电流（单位：mA）
 * @param[in] u16_Carrier                   载波半周期时钟数（carrier_clk）
 * @param[in] u32_Silent_Time               静默时间时钟数
 * @param[in] u16_Interval                  包络更新间隔时钟数（am_interval）
 *
 * @note 该模式仅输出正半波（negative_enable = 0），通过交替方向实现交流效果
 *
 * @see nnc6521_customized_waveform()
 * @see nnc6521_wavegen_config()
 */
void nnc6521_amplitude_modulation(uint8_t chip_id,
                                  uint8_t u8_Channel,
                                  uint8_t u8_PointNum,
                                  float *f_Normalized_Envelope_array,
                                  uint32_t u32_Max_current,
                                  uint16_t u16_Carrier,
                                  uint32_t u32_Silent_Time,
                                  uint16_t u16_Interval)
{
    waveform_TypeDef wf = {0};
    wf.CHANNEL = u8_Channel;
    wf.WG_DRV_POINT_CONFIG.value = u8_PointNum;
    wf.WG_DRV_CONFIG_REG0.bits.rest_enable = 1;        /* 死区使能 */
    wf.WG_DRV_CONFIG_REG0.bits.negative_enable = 0;    /* 禁用负半波（AM 模式） */
    wf.WG_DRV_CONFIG_REG0.bits.silent_enable = 1;      /* 静默期使能 */
    wf.WG_DRV_CONFIG_REG0.bits.sourceB_enable = 1;     /* Source B 使能 */
    wf.WG_DRV_CONFIG_REG0.bits.multi_electrode = 1;    /* 多电极模式 */
    wf.WG_DRV_CONFIG_REG0.bits.alternating_pos = 1;    /* 交替方向输出 */
    wf.WG_DRV_SILENT_CLK.value = u32_Silent_Time;
    wf.WG_DRV_HALF_WAVE_CLK_PNT.value = u16_Interval;  /* 包络更新间隔 */
    wf.WG_DRV_REST_CLK.value = 0x00;                   /* 无死区 */
    wf.WG_DRV_CTRL_REG0.bits.enable_wavegen = 1;       /* 使能波形发生器 */
    wf.WG_DRV_CTRL_REG0.bits.waveform_select = WAVEFORM_SPI;  /* SPI 模式 */
    wf.WG_DRV_CTRL_REG0.bits.symmetric_wave = 1;       /* 对称模式 */
    wf.WG_DRV_ALT_LIM.value = u16_Carrier;              /* 载波半周期 */
    wf.WG_DRV_ALT_SILENT_LIM.value = 0x0000;            /* 无载波静默 */

    nnc6521_wavegen_config(chip_id, &wf, f_Normalized_Envelope_array, u32_Max_current);
}

/**
 * @brief 导联脱落检测 (LOD) 寄存器默认初始化
 *
 * 将 LOD 相关寄存器写入默认值，在 nnc6521_lod_init() 中首先调用。
 * 涵盖控制寄存器、高低阈值、延迟目标、中断和模拟配置。
 *
 * @param[in] chip_id 芯片编号
 *
 * @see nnc6521_lod_init()
 * @see nnc6521_leadoff_config()
 */
static void nnc6521_leadoff_init(uint8_t chip_id)
{
    nnc6521_write_reg(chip_id, LEAD_OFF_CTRL_ADDR, 0x30);         /* LOD 控制寄存器 */
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_H_0_ADDR, 0xF0);     /* 高阈值低字节 */
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_H_1_ADDR, 0x00);     /* 高阈值高字节 */
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_L_0_ADDR, 0x10);     /* 低阈值低字节 */
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_L_1_ADDR, 0x10);     /* 低阈值高字节 */
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_0_ADDR, 0x00);   /* 延迟目标字节 0 */
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_1_ADDR, 0x00);   /* 延迟目标字节 1 */
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_2_ADDR, 0x00);   /* 延迟目标字节 2 */
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_3_ADDR, 0x00);   /* 延迟目标字节 3 */
    nnc6521_write_reg(chip_id, LEAD_OFF_TGT_ADDR, 0x10);         /* 脱落检测目标计数 */
    nnc6521_write_reg(chip_id, LEAD_OFF_INT_ADDR, 0x10);         /* 中断配置 */
    nnc6521_write_reg(chip_id, LEAD_OFF_ANA_ADDR, 0x00);         /* 模拟前端配置 */
}

/**
 * @brief 写入导联脱落检测 (LOD) 完整配置
 *
 * 将 Leadoff_TypeDef 结构体中的所有配置写入对应寄存器，包括：
 * - LOD 控制、阈值、延迟、中断寄存器
 * - 根据 dac_sel 配置 CH1/CH2 的 VDAC、比较器和驱动放大器
 *
 * @param[in] chip_id 芯片编号
 * @param[in] LO      LOD 配置结构体指针
 *
 * @see nnc6521_lod_init()
 */
static void nnc6521_leadoff_config(uint8_t chip_id, Leadoff_TypeDef *LO)
{
    uint8_t data;

    /* 写入 LOD 核心寄存器 */
    nnc6521_write_reg(chip_id, LEAD_OFF_CTRL_ADDR, LO->lead_off_ctrl.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_H_0_ADDR, LO->lead_off_thr_h_0.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_H_1_ADDR, LO->lead_off_thr_h_1.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_L_0_ADDR, LO->lead_off_thr_l_0.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_THR_L_1_ADDR, LO->lead_off_thr_l_1.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_0_ADDR, LO->lead_off_dly_tgt[0].all);
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_1_ADDR, LO->lead_off_dly_tgt[1].all);
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_2_ADDR, LO->lead_off_dly_tgt[2].all);
    nnc6521_write_reg(chip_id, LEAD_OFF_DLY_TGT_3_ADDR, LO->lead_off_dly_tgt[3].all);
    nnc6521_write_reg(chip_id, LEAD_OFF_TGT_ADDR, LO->lead_off_tgt.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_INT_ADDR, LO->lead_off_int.all);
    nnc6521_write_reg(chip_id, LEAD_OFF_ANA_ADDR, LO->lead_off_ana.all);

    /* CH1 VDAC + 比较器 + 驱动放大器配置 */
    if ((LO->lead_off_ctrl.bits.dac_sel & 0x01) == 1) {
        data = nnc6521_read_reg(chip_id, ANA_INTR_EN_ADDR) & ~(1 << 1);
        nnc6521_write_reg(chip_id, ANA_INTR_EN_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_GEN_REG_2_ADDR) | LO->ANA_GEN_REG_2.all;
        nnc6521_write_reg(chip_id, ANA_GEN_REG_2_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_GEN_REG_3_ADDR) | LO->ANA_GEN_REG_3.all;
        nnc6521_write_reg(chip_id, ANA_GEN_REG_3_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_ENABLE_REG_1_ADDR) | LO->ANA_ENABLE_REG_1.all;
        nnc6521_write_reg(chip_id, ANA_ENABLE_REG_1_ADDR, data);
    }

    /* CH2 VDAC + 比较器 + 驱动放大器配置 */
    if ((LO->lead_off_ctrl.bits.dac_sel & 0x02) == 2) {
        data = nnc6521_read_reg(chip_id, ANA_INTR_EN_ADDR) & ~(1 << 2);
        nnc6521_write_reg(chip_id, ANA_INTR_EN_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_GEN_REG_4_ADDR) | LO->ANA_GEN_REG_4.all;
        nnc6521_write_reg(chip_id, ANA_GEN_REG_4_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_GEN_REG_5_ADDR) | LO->ANA_GEN_REG_5.all;
        nnc6521_write_reg(chip_id, ANA_GEN_REG_5_ADDR, data);

        data = nnc6521_read_reg(chip_id, ANA_ENABLE_REG_2_ADDR) | LO->ANA_ENABLE_REG_2.all;
        nnc6521_write_reg(chip_id, ANA_ENABLE_REG_2_ADDR, data);
    }
}

/**
 * @brief 导联脱落检测 (LOD) 完整初始化
 *
 * 执行完整的 LOD 初始化流程：
 * 1. 调用 nnc6521_leadoff_init() 写入默认寄存器值
 * 2. 配置 Leadoff_TypeDef 结构体：
 *    - 使能 CH1 + CH2 双通道 VDAC（dac_sel = 0x03）
 *    - 高电平检测模式（check_mode = 0x01）
 *    - 设置高阈值、延迟目标、目标计数
 *    - 配置 CH1/CH2 的 VDAC 振幅斩波、比较器、驱动放大器
 * 3. 调用 nnc6521_leadoff_config() 写入配置
 * 4. 清除 LOD 中断标志
 *
 * @param[in] chip_id           芯片编号
 * @param[in] u16_LOD_Threshold 脱落检测高阈值（16-bit）
 * @param[in] u32_LOD_DLY       脱落检测延迟目标（32-bit，4 字节拆分写入）
 * @param[in] u8_LOD_TGT        脱落检测目标计数
 *
 * @see nnc6521_leadoff_init()
 * @see nnc6521_leadoff_config()
 * @see nnc6521_clear_lod_int()
 */
void nnc6521_lod_init(uint8_t chip_id,
                      uint16_t u16_LOD_Threshold,
                      uint32_t u32_LOD_DLY,
                      uint8_t u8_LOD_TGT)
{
    nnc6521_leadoff_init(chip_id);

    Leadoff_TypeDef lo = {0};
    lo.lead_off_ctrl.bits.dac_sel = 0x03;       /* LO_ALL: CH1 + CH2 VDAC 均使能 */
    lo.lead_off_ctrl.bits.check_mode = 0x01;    /* 高电平检测模式 */
    lo.lead_off_int.bits.lead_off_int_en = 0;   /* 禁用 LOD 中断 */

    /* 拆分 16-bit 阈值到两个字节寄存器 */
    lo.lead_off_thr_h_0.all = u16_LOD_Threshold & 0x00FF;
    lo.lead_off_thr_h_1.all = u16_LOD_Threshold >> 8;

    /* 拆分 32-bit 延迟到 4 个字节寄存器 */
    lo.lead_off_dly_tgt[0].all = (uint8_t)(u32_LOD_DLY & 0xFF);
    lo.lead_off_dly_tgt[1].all = (uint8_t)((u32_LOD_DLY >> 8) & 0xFF);
    lo.lead_off_dly_tgt[2].all = (uint8_t)((u32_LOD_DLY >> 16) & 0xFF);
    lo.lead_off_dly_tgt[3].all = (uint8_t)((u32_LOD_DLY >> 24) & 0xFF);
    lo.lead_off_tgt.all = u8_LOD_TGT;

    /* CH1 模拟前端配置 */
    lo.ANA_GEN_REG_2.all = 0x04;
    lo.ANA_GEN_REG_3.bits.D2A_VDAC_AMPCHOP_CH1 = 1;    /* CH1 VDAC 振幅斩波使能 */
    lo.ANA_GEN_REG_3.bits.COMP_ISEL_CH1 = 0;           /* CH1 比较器电流选择 */
    lo.ANA_GEN_REG_3.bits.DRIVERA_CSAMP_CH_CH1 = 1;    /* CH1 驱动放大器电流采样 */
    lo.ANA_GEN_REG_3.bits.VDAC_DIN_CH1_MSB = 0;        /* CH1 VDAC 数据高位 */
    lo.ANA_ENABLE_REG_1.bits.VDAC_EN_CH1 = 1;           /* CH1 VDAC 使能 */
    lo.ANA_ENABLE_REG_1.bits.COMP_EN_CH1 = 1;           /* CH1 比较器使能 */
    lo.ANA_ENABLE_REG_1.bits.DRIVERA_CSAMP_EN_CH1 = 1;  /* CH1 驱动放大器使能 */

    /* CH2 模拟前端配置 */
    lo.ANA_GEN_REG_4.all = 0x04;
    lo.ANA_GEN_REG_5.bits.D2A_VDAC_AMPCHOP_CH2 = 1;    /* CH2 VDAC 振幅斩波使能 */
    lo.ANA_GEN_REG_5.bits.COMP_ISEL_CH2 = 0;           /* CH2 比较器电流选择 */
    lo.ANA_GEN_REG_5.bits.DRIVERA_CSAMP_CH_CH2 = 1;    /* CH2 驱动放大器电流采样 */
    lo.ANA_GEN_REG_5.bits.VDAC_DIN_CH2_MSB = 0;        /* CH2 VDAC 数据高位 */
    lo.ANA_ENABLE_REG_2.bits.VDAC_EN_CH2 = 1;           /* CH2 VDAC 使能 */
    lo.ANA_ENABLE_REG_2.bits.COMP_EN_CH2 = 1;           /* CH2 比较器使能 */
    lo.ANA_ENABLE_REG_2.bits.DRIVERA_CSAMP_EN_CH2 = 1;  /* CH2 驱动放大器使能 */

    nnc6521_leadoff_config(chip_id, &lo);
    nnc6521_clear_lod_int(chip_id);
}

/**
 * @brief 写入短路检测 (SCD) 完整配置
 *
 * 将 Short_detect_TypeDef 结构体中的所有配置写入对应寄存器，包括：
 * - LOD 控制寄存器（复用 LOD 控制地址）
 * - 模拟前端使能寄存器（CH1/CH2/全局）
 * - VDAC 和比较器通用配置寄存器
 * - 比较器极性和波形停止控制
 * - 中断地址映射（SIM0~SIM3）和中断计数
 *
 * @param[in] chip_id 芯片编号
 * @param[in] SO      SCD 配置结构体指针
 *
 * @see nnc6521_scd_init()
 */
static void nnc6521_short_detect_config(uint8_t chip_id, Short_detect_TypeDef *SO)
{
    uint8_t data;

    /* LOD 控制寄存器（SCD 复用该地址） */
    nnc6521_write_reg(chip_id, LEAD_OFF_CTRL_ADDR, SO->lead_off_ctrl.all);

    /* 模拟前端使能寄存器 */
    data = nnc6521_read_reg(chip_id, ANA_ENABLE_REG_1_ADDR) | SO->ANA_ENABLE_REG_1.all;
    nnc6521_write_reg(chip_id, ANA_ENABLE_REG_1_ADDR, data);

    data = nnc6521_read_reg(chip_id, ANA_ENABLE_REG_2_ADDR) | SO->ANA_ENABLE_REG_2.all;
    nnc6521_write_reg(chip_id, ANA_ENABLE_REG_2_ADDR, data);

    nnc6521_write_reg(chip_id, ANA_ENABLE_REG_3_ADDR, SO->ANA_ENABLE_REG_3.all);

    /* VDAC 和比较器通用配置寄存器 */
    data = nnc6521_read_reg(chip_id, ANA_GEN_REG_2_ADDR) | SO->ANA_GEN_REG_2.all;
    nnc6521_write_reg(chip_id, ANA_GEN_REG_2_ADDR, data);

    data = nnc6521_read_reg(chip_id, ANA_GEN_REG_3_ADDR) | SO->ANA_GEN_REG_3.all;
    nnc6521_write_reg(chip_id, ANA_GEN_REG_3_ADDR, data);

    data = nnc6521_read_reg(chip_id, ANA_GEN_REG_4_ADDR) | SO->ANA_GEN_REG_4.all;
    nnc6521_write_reg(chip_id, ANA_GEN_REG_4_ADDR, data);

    data = nnc6521_read_reg(chip_id, ANA_GEN_REG_5_ADDR) | SO->ANA_GEN_REG_5.all;
    nnc6521_write_reg(chip_id, ANA_GEN_REG_5_ADDR, data);

    /* 比较器极性与波形停止控制 */
    nnc6521_write_reg(chip_id, ANA_INT_COMP_POL_ADDR, SO->ANA_INT_COMP_POL.all);
    nnc6521_write_reg(chip_id, ANA_INT_STOP_WAVEGEN_ADDR, SO->ANA_INT_STOP_WAVEGEN.all);

    /* CH1 中断地址映射（SIM0、SIM1） */
    nnc6521_write_reg(chip_id, ANA_INT_SIM0_A00_ADDR, SO->ANA_INT_SIM0_A00.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM0_A01_ADDR, SO->ANA_INT_SIM0_A01.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM1_A10_ADDR, SO->ANA_INT_SIM1_A10.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM1_A11_ADDR, SO->ANA_INT_SIM1_A11.all);

    nnc6521_write_reg(chip_id, ANA_INT_CH1_INT_NUMBER_ADDR, SO->ANA_INT_CH1_INT_NUMBER.all);

    /* CH2 中断地址映射（SIM2、SIM3） */
    nnc6521_write_reg(chip_id, ANA_INT_SIM2_A20_ADDR, SO->ANA_INT_SIM2_A20.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM2_A21_ADDR, SO->ANA_INT_SIM2_A21.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM3_A30_ADDR, SO->ANA_INT_SIM3_A30.all);
    nnc6521_write_reg(chip_id, ANA_INT_SIM3_A31_ADDR, SO->ANA_INT_SIM3_A31.all);

    nnc6521_write_reg(chip_id, ANA_INT_CH2_INT_NUMBER_ADDR, SO->ANA_INT_CH2_INT_NUMBER.all);
    nnc6521_write_reg(chip_id, ANA_INTR_SIM_CL_ADDR, SO->ANA_INTR_SIM_CL.all);
}

/**
 * @brief 短路检测 (SCD) 完整初始化
 *
 * 配置 CH1 和 CH2 的短路检测参数：
 * - DAC 阈值：用于比较器判断是否发生短路
 * - 中断地址：指定波形 RAM 中的检测位置
 * - 中断计数：触发短路中断所需的匹配次数
 *
 * 检测原理：VDAC 输出与外部电极电压比较，当差值超过阈值时判定为短路。
 *
 * @param[in] chip_id              芯片编号
 * @param[in] u8_CH0_DAC_Threshold CH1 短路检测 DAC 阈值
 * @param[in] u8_CH0_Addr_0        CH1 中断地址 0（波形 RAM 起始位置）
 * @param[in] u8_CH0_Addr_1        CH1 中断地址 1（波形 RAM 结束位置）
 * @param[in] u8_CH0_Int_Num       CH1 中断计数阈值
 * @param[in] u8_CH1_DAC_Threshold CH2 短路检测 DAC 阈值
 * @param[in] u8_CH1_Addr_0        CH2 中断地址 0
 * @param[in] u8_CH1_Addr_1        CH2 中断地址 1
 * @param[in] u8_CH1_Int_Num       CH2 中断计数阈值
 *
 * @see nnc6521_short_detect_config()
 * @see nnc6521_clear_scd_int()
 */
void nnc6521_scd_init(uint8_t chip_id,
                      uint8_t u8_CH0_DAC_Threshold,
                      uint8_t u8_CH0_Addr_0,
                      uint8_t u8_CH0_Addr_1,
                      uint8_t u8_CH0_Int_Num,
                      uint8_t u8_CH1_DAC_Threshold,
                      uint8_t u8_CH1_Addr_0,
                      uint8_t u8_CH1_Addr_1,
                      uint8_t u8_CH1_Int_Num)
{
    Short_detect_TypeDef so = {0};
    so.lead_off_ctrl.bits.dac_sel = 0x03;  /* LO_ALL: CH1 + CH2 均使能 */

    /* CH1 短路检测配置 */
    so.ANA_GEN_REG_2.all = u8_CH0_DAC_Threshold;
    so.ANA_GEN_REG_3.bits.D2A_VDAC_AMPCHOP_CH1 = 1;    /* CH1 VDAC 振幅斩波使能 */
    so.ANA_GEN_REG_3.bits.COMP_ISEL_CH1 = 0;           /* CH1 比较器电流选择 */
    so.ANA_ENABLE_REG_1.bits.VDAC_EN_CH1 = 1;           /* CH1 VDAC 使能 */
    so.ANA_ENABLE_REG_1.bits.D2A_STIMU_COMP_EN_CH1 = 1; /* CH1 刺激比较器使能 */
    so.ANA_ENABLE_REG_1.bits.D2A_STIMU_COMP_SEL_CH1 = 0; /* CH1 比较器选择 */
    so.ANA_INT_SIM0_A00.all = u8_CH0_Addr_0;            /* CH1 中断地址 0 */
    so.ANA_INT_SIM0_A01.all = u8_CH0_Addr_1;            /* CH1 中断地址 1 */
    so.ANA_INT_CH1_INT_NUMBER.all = u8_CH0_Int_Num;     /* CH1 中断计数 */
    so.ANA_INT_COMP_POL.bits.ANA_STIMU_CH1_INTR_EN = 0;  /* CH1 中断暂不使能 */

    /* CH2 短路检测配置 */
    so.ANA_GEN_REG_4.all = u8_CH1_DAC_Threshold;
    so.ANA_GEN_REG_5.bits.D2A_VDAC_AMPCHOP_CH2 = 1;    /* CH2 VDAC 振幅斩波使能 */
    so.ANA_GEN_REG_5.bits.COMP_ISEL_CH2 = 0;           /* CH2 比较器电流选择 */
    so.ANA_ENABLE_REG_2.bits.VDAC_EN_CH2 = 1;           /* CH2 VDAC 使能 */
    so.ANA_ENABLE_REG_2.bits.D2A_STIMU_COMP_EN_CH2 = 1; /* CH2 刺激比较器使能 */
    so.ANA_ENABLE_REG_2.bits.D2A_STIMU_COMP_SEL_CH2 = 0; /* CH2 比较器选择 */
    so.ANA_INT_SIM2_A20.all = u8_CH1_Addr_0;            /* CH2 中断地址 0 */
    so.ANA_INT_SIM2_A21.all = u8_CH1_Addr_1;            /* CH2 中断地址 1 */
    so.ANA_INT_CH2_INT_NUMBER.all = u8_CH1_Int_Num;     /* CH2 中断计数 */
    so.ANA_INT_COMP_POL.bits.ANA_STIMU_CH2_INTR_EN = 0;  /* CH2 中断暂不使能 */

    nnc6521_short_detect_config(chip_id, &so);
    nnc6521_clear_scd_int(chip_id);
}

/**
 * @brief 清除短路检测 (SCD) 中断标志
 *
 * 向 ANA_INTR_SIM_CL_ADDR 写入 0x03，清除 CH1 和 CH2 的短路中断标志。
 *
 * @param[in] chip_id 芯片编号
 *
 * @see nnc6521_scd_init()
 */
void nnc6521_clear_scd_int(uint8_t chip_id)
{
    nnc6521_write_reg(chip_id, ANA_INTR_SIM_CL_ADDR, 0x03);
}

/**
 * @brief 清除导联脱落检测 (LOD) 中断标志
 *
 * 通过先清零再置位 bit1 实现中断清除（写 1 清除方式）：
 * 1. 读取当前 LEAD_OFF_INT_ADDR 值
 * 2. 清除 bit1
 * 3. 置位 bit1（触发清除动作）
 *
 * @param[in] chip_id 芯片编号
 *
 * @see nnc6521_lod_init()
 */
void nnc6521_clear_lod_int(uint8_t chip_id)
{
    uint8_t data;
    data = nnc6521_read_reg(chip_id, LEAD_OFF_INT_ADDR);
    nnc6521_write_reg(chip_id, LEAD_OFF_INT_ADDR, data & ~(0x02));
    nnc6521_write_reg(chip_id, LEAD_OFF_INT_ADDR, data | 0x02);
}

/**
 * @brief 波形发生器完整寄存器配置流程
 *
 * 这是 NNC6521 波形输出的核心函数，所有波形输出最终都通过此函数完成配置。
 * 寄存器写入顺序如下（顺序不可随意更改）：
 *
 * 1. **禁用 AWG**：先禁用波形发生器，避免配置过程中产生异常输出
 * 2. **使能模拟通道**：ANA_ENABLE_REG_1/2 写入 0x09
 * 3. **采样点数**：WG_DRV_POINT_CONFIG — 每半波采样点数
 * 4. **配置寄存器**：WG_DRV_CONFIG_REG0 — 死区/负半波/静默/SourceB/多电极/循环
 * 5. **静默时间**：WG_DRV_SILENT_CLK（24-bit，3 字节写入）
 * 6. **正半波间隔**：WG_DRV_HALF_WAVE_CLK_PNT（16-bit）— 控制波形频率
 * 7. **负半波间隔**：WG_DRV_NEG_HALF_WAVE_CLK_PNT（16-bit）
 * 8. **驱动电流**：WG_DRIVE_REG_CTRL2 — CI 档位和 scale_up
 * 9. **死区时间**：WG_DRV_REST_CLK（16-bit）
 * 10. **中断计数**：WG_DRV_INT_NUM_WAVE
 * 11. **中断控制**：WG_DRV_INT_REG01
 * 12. **中断地址**：WG_DRV_INT_REG02、WG_DRV_INT_REG03
 * 13. **波形数据**（仅 SPI 模式）：
 *     - 读回点数确认
 *     - Current_Output() 将目标电流转为 12-bit DAC 数值
 *     - generate_scaled_wave() 缩放归一化数据
 *     - convert_16bit_to_8bit() 适配 8-bit SPI 传输
 *     - 逐点写入波形 RAM（WG_DRV_IN_WAVE_ADDR + WG_DRV_IN_WAVE）
 * 14. **交替模式**：WG_DRV_ALT_LIM（载波周期）、WG_DRV_ALT_SILENT_LIM（载波静默）
 * 15. **使能 AWG**：最后写 WG_DRV_CTRL_REG0，包含波形选择和使能位
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] wf                        波形配置结构体（所有寄存器值已填充）
 * @param[in] normalized_waveform_array  归一化波形数据（SPI 模式时使用，预加载模式传 NULL）
 * @param[in] max_current               最大输出电流（单位：mA，SPI 模式时使用）
 *
 * @note 预加载模式（waveform_select != WAVEFORM_SPI）跳过波形数据传输步骤
 * @note SPI 模式下会修改 wf->WG_DRIVE_REG_CTRL2.bits.out_pos（scale_up）
 *
 * @see nnc6521_awg_enable_disable()
 * @see nnc6521_preloaded_waveform()
 * @see nnc6521_customized_waveform()
 * @see nnc6521_amplitude_modulation()
 */
void nnc6521_wavegen_config(uint8_t chip_id,
                            waveform_TypeDef *wf,
                            float *normalized_waveform_array,
                            uint32_t max_current)
{
    int addr;
    uint8_t i = 0;
    uint8_t driver_point = 0;

    /* 步骤 1：禁用 AWG，避免配置过程中产生异常输出 */
    nnc6521_awg_enable_disable(chip_id, wf->CHANNEL, 0);

    /* 步骤 2：使能对应通道的模拟前端 */
    if (wf->CHANNEL == WAVEFORM_GEN_CH0) {
        nnc6521_write_reg(chip_id, ANA_ENABLE_REG_1_ADDR, 0x09);
    } else {
        nnc6521_write_reg(chip_id, ANA_ENABLE_REG_2_ADDR, 0x09);
    }

    /* 步骤 3：采样点数寄存器 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_POINT_CONFIG_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_POINT_CONFIG.value);

    /* 步骤 4：配置寄存器（死区、负半波、静默、SourceB、多电极、循环） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_CONFIG_REG0_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_CONFIG_REG0.value);

    /* 步骤 5：静默时间寄存器（24-bit，分 3 字节写入） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_SILENT_CLK_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_SILENT_CLK.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 0x01, (wf->WG_DRV_SILENT_CLK.value >> 8) & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 0x02, (wf->WG_DRV_SILENT_CLK.value >> 16) & 0xFF);

    /* 步骤 6：正半波间隔寄存器（16-bit） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_HALF_WAVE_CLK_PNT_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_HALF_WAVE_CLK_PNT.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 1, (wf->WG_DRV_HALF_WAVE_CLK_PNT.value >> 8) & 0xFF);

    /* 步骤 7：负半波间隔寄存器（16-bit） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_NEG_HALF_WAVE_CLK_PNT_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_NEG_HALF_WAVE_CLK_PNT.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 1, (wf->WG_DRV_NEG_HALF_WAVE_CLK_PNT.value >> 8) & 0xFF);

    /* 步骤 8：驱动电流寄存器 2 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRIVE_REG_CTRL2_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRIVE_REG_CTRL2.value);

    /* 步骤 9：死区时间寄存器（16-bit） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_REST_CLK_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_REST_CLK.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 1, (wf->WG_DRV_REST_CLK.value >> 8) & 0xFF);

    /* 步骤 10：中断计数寄存器 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_INT_NUM_WAVE_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_INT_NUM_WAVE.value);

    /* 步骤 11：中断控制寄存器 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_INT_REG01_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_INT_REG01.value);

    /* 步骤 12：中断地址寄存器 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_INT_REG02_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_INT_REG02.value);

    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_INT_REG03_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_INT_REG03.value);

    /* 步骤 13：波形数据处理（仅 SPI 自定义波形模式） */
    if (wf->WG_DRV_CTRL_REG0.bits.waveform_select == WAVEFORM_SPI) {
        /* 读回采样点数确认 */
        addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_POINT_CONFIG_OFFSET);
        wf->WG_DRV_POINT_CONFIG.value = nnc6521_read_wave_reg(chip_id, addr);

        /* 电流校准流程：目标电流 → 12-bit DAC → 缩放 → 8-bit 适配 */
        uint16_t calibrated_CurrentArray_16bits[wf->WG_DRV_POINT_CONFIG.value];
        uint8_t calibrated_CurrentArray_8bits[wf->WG_DRV_POINT_CONFIG.value];
        uint8_t scale_up = 0;
        uint16_t max_amplitude = 0;

        max_amplitude = Current_Output(chip_id, max_current, wf->CHANNEL);
        generate_scaled_wave(calibrated_CurrentArray_16bits,
                             wf->WG_DRV_POINT_CONFIG.value,
                             normalized_waveform_array, max_amplitude);
        convert_16bit_to_8bit(calibrated_CurrentArray_8bits, &scale_up,
                             calibrated_CurrentArray_16bits,
                             wf->WG_DRV_POINT_CONFIG.value);

        /* 更新驱动电流寄存器 2 的 scale_up 因子 */
        addr = WG_REG_ADDR(wf->CHANNEL, WG_DRIVE_REG_CTRL2_OFFSET);
        wf->WG_DRIVE_REG_CTRL2.bits.out_pos = 0x04 - scale_up;
        nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRIVE_REG_CTRL2.value);

        if (wf->WG_DRV_POINT_CONFIG.value > 0) {
            /* 计算实际写入点数（预加载模式需翻倍） */
            if (wf->WG_DRV_CTRL_REG0.bits.preload_mode == 0) {
                driver_point = wf->WG_DRV_POINT_CONFIG.value;
            } else {
                driver_point = wf->WG_DRV_POINT_CONFIG.value * 2;
            }

            /* 逐点写入波形 RAM */
            if (normalized_waveform_array != NULL) {
                for (i = 0; i < driver_point; i++) {
                    nnc6521_write_wave_reg(chip_id,
                        WG_REG_ADDR(wf->CHANNEL, WG_DRV_IN_WAVE_ADDR_OFFSET), i);
                    nnc6521_write_wave_reg(chip_id,
                        WG_REG_ADDR(wf->CHANNEL, WG_DRV_IN_WAVE_OFFSET),
                        *(calibrated_CurrentArray_8bits + i));
                }
            }
        }
    } else {
        /* 预加载波形模式：仅更新驱动电流 */
        addr = WG_REG_ADDR(wf->CHANNEL, WG_DRIVE_REG_CTRL2_OFFSET);
        nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRIVE_REG_CTRL2.value);
    }

    /* 步骤 14：交替模式载波周期寄存器（16-bit） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_ALT_LIM_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_ALT_LIM.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 1, (wf->WG_DRV_ALT_LIM.value >> 8) & 0xFF);

    /* 步骤 14：交替模式静默间隔寄存器（16-bit） */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_ALT_SILENT_LIM_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_ALT_SILENT_LIM.value & 0xFF);
    nnc6521_write_wave_reg(chip_id, addr + 1,
        (wf->WG_DRV_ALT_SILENT_LIM.value >> 8) & 0xFF);

    /* 步骤 15：最后写入控制寄存器，使能波形发生器 */
    addr = WG_REG_ADDR(wf->CHANNEL, WG_DRV_CTRL_REG0_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, wf->WG_DRV_CTRL_REG0.value);
}

/**
 * @brief 更新波形幅度（适用于中断上下文实时更新）
 *
 * 在波形输出过程中动态更新整个波形的幅度，不重新配置时序参数。
 * 适用于需要在定时器中断中实时调整波形幅度的场景。
 *
 * 处理流程：
 * 1. Current_Output() 将目标电流转为 12-bit DAC 数值
 * 2. generate_scaled_wave() 缩放归一化数据
 * 3. convert_16bit_to_8bit() 适配 8-bit SPI 传输
 * 4. 更新 scale_up 到驱动电流寄存器
 * 5. 逐点写入波形 RAM
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] Channel                   通道编号
 * @param[in] u8_PointNum               波形采样点数
 * @param[in] normalized_waveform_array  归一化波形数据（0.0~1.0）
 * @param[in] max_current               最大输出电流（单位：mA）
 *
 * @note 该函数不修改时序参数，仅更新波形数据和幅度
 * @note 适用于中断上下文，函数体较小且无复杂分支
 *
 * @see nnc6521_customized_amplitude_addr()
 * @see nnc6521_customized_waveform()
 */
void nnc6521_customized_amplitude(uint8_t chip_id,
                                  uint8_t Channel,
                                  uint8_t u8_PointNum,
                                  float *normalized_waveform_array,
                                  uint32_t max_current)
{
    int addr;
    uint8_t i = 0;

    uint16_t calibrated_CurrentArray_16bits[u8_PointNum];
    uint8_t calibrated_CurrentArray_8bits[u8_PointNum];
    uint8_t scale_up = 0;
    uint16_t max_amplitude = 0;

    /* 电流校准：目标电流 → DAC 数值 → 缩放 → 8-bit 适配 */
    max_amplitude = Current_Output(chip_id, max_current, Channel);
    generate_scaled_wave(calibrated_CurrentArray_16bits, u8_PointNum,
                         normalized_waveform_array, max_amplitude);
    convert_16bit_to_8bit(calibrated_CurrentArray_8bits, &scale_up,
                         calibrated_CurrentArray_16bits, u8_PointNum);

    /* 更新 scale_up 到驱动电流寄存器 */
    addr = WG_REG_ADDR(Channel, WG_DRIVE_REG_CTRL2_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, 0x04 - scale_up);

    /* 逐点写入波形 RAM */
    if (normalized_waveform_array != NULL) {
        for (i = 0; i < u8_PointNum; i++) {
            nnc6521_write_wave_reg(chip_id,
                WG_REG_ADDR(Channel, WG_DRV_IN_WAVE_ADDR_OFFSET), i);
            nnc6521_write_wave_reg(chip_id,
                WG_REG_ADDR(Channel, WG_DRV_IN_WAVE_OFFSET),
                *(calibrated_CurrentArray_8bits + i));
        }
    }
}

/**
 * @brief 更新指定地址范围的波形幅度
 *
 * 与 nnc6521_customized_amplitude() 类似，但仅更新波形 RAM 中
 * [u8_Start_Address, u8_End_Address) 范围内的数据，其余点保持不变。
 * 适用于需要局部更新波形的场景（如渐变包络）。
 *
 * @note 该函数放置在 .nnc6521_amfunc 段，可在 RAM 中执行以提高中断响应速度
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] Channel                   通道编号
 * @param[in] u8_Start_Address          波形 RAM 起始地址（包含）
 * @param[in] u8_End_Address            波形 RAM 结束地址（不包含）
 * @param[in] normalized_waveform_array  归一化波形数据（0.0~1.0）
 * @param[in] max_current               最大输出电流（单位：mA）
 *
 * @see nnc6521_customized_amplitude()
 */
__attribute__((section(".nnc6521_amfunc")))
void nnc6521_customized_amplitude_addr(uint8_t chip_id,
                                       uint8_t Channel,
                                       uint8_t u8_Start_Address,
                                       uint8_t u8_End_Address,
                                       float *normalized_waveform_array,
                                       uint32_t max_current)
{
    int addr;
    uint8_t i = 0, j = 0;
    uint8_t u8_PointNum = u8_End_Address - u8_Start_Address;

    uint16_t calibrated_CurrentArray_16bits[u8_PointNum];
    uint8_t calibrated_CurrentArray_8bits[u8_PointNum];
    uint8_t scale_up = 0;
    uint16_t max_amplitude = 0;

    /* 电流校准 */
    max_amplitude = Current_Output(chip_id, max_current, Channel);
    generate_scaled_wave(calibrated_CurrentArray_16bits, u8_PointNum,
                         normalized_waveform_array, max_amplitude);
    convert_16bit_to_8bit(calibrated_CurrentArray_8bits, &scale_up,
                         calibrated_CurrentArray_16bits, u8_PointNum);

    /* 更新 scale_up */
    addr = WG_REG_ADDR(Channel, WG_DRIVE_REG_CTRL2_OFFSET);
    nnc6521_write_wave_reg(chip_id, addr, 0x04 - scale_up);

    /* 仅写入指定地址范围 */
    if (normalized_waveform_array != NULL) {
        for (i = u8_Start_Address, j = 0; i < u8_End_Address; i++, j++) {
            nnc6521_write_wave_reg(chip_id,
                WG_REG_ADDR(Channel, WG_DRV_IN_WAVE_ADDR_OFFSET), i);
            nnc6521_write_wave_reg(chip_id,
                WG_REG_ADDR(Channel, WG_DRV_IN_WAVE_OFFSET),
                *(calibrated_CurrentArray_8bits + j));
        }
    }
}

/* ============================================================================
 *  电流校准函数（保留自原始 ens1p4.c）
 * ===========================================================================*/

/**
 * @brief 将归一化波形数据缩放到目标幅度
 *
 * 计算公式：output[i] = base_wave[i] * amplitude，结果限制在 0~4095 范围内。
 * 使用 +0.5f 四舍五入取整。
 *
 * @param[out] output      输出的 16-bit 电流值数组
 * @param[in]  u8_PointNum 采样点数
 * @param[in]  base_wave   归一化浮点波形数据（0.0~1.0）
 * @param[in]  amplitude   目标幅度值
 */
static void generate_scaled_wave(uint16_t *output, uint8_t u8_PointNum,
                                 float *base_wave, float amplitude)
{
    for (int i = 0; i < u8_PointNum; i++) {
        float value = base_wave[i] * amplitude;
        if (value > 4095.0f) value = 4095.0f;  /* 12-bit DAC 满量程限制 */
        output[i] = (uint16_t)(value + 0.5f);  /* 四舍五入 */
    }
}

/**
 * @brief 将 16-bit 数组转换为 8-bit 数组（带自适应移位）
 *
 * 根据最大值自动计算右移量：
 * - 若最大值 ≤ 255（8-bit 可表示），不移位
 * - 否则取最大值位数对 8 取模作为右移量
 *
 * @param[out] dst        输出的 8-bit 数组
 * @param[out] bits_shift 实际右移位数
 * @param[in]  src        输入的 16-bit 数组
 * @param[in]  length     数组长度
 */
static void convert_16bit_to_8bit(uint8_t *dst, uint8_t *bits_shift,
                                  uint16_t *src, int length)
{
    uint16_t max_amp = 0;
    for (int i = 0; i < length; i++) {
        if (src[i] > max_amp) max_amp = src[i];
    }

    uint8_t used = bits_used(max_amp);
    if (used <= 8) {
        *bits_shift = 0;        /* 8-bit 可表示，无需移位 */
    } else {
        *bits_shift = used % 8; /* 取模得到额外位数 */
    }

    for (int i = 0; i < length; i++) {
        dst[i] = (uint8_t)((src[i] >> (*bits_shift)) & 0xFF);
    }
}

/**
 * @brief 计算 16-bit 值实际占用的位数
 *
 * @param[in] value 输入数值
 * @return 位数（0 返回 0）
 */
static uint8_t bits_used(uint16_t value)
{
    uint8_t bits = 0;
    while (value > 0) {
        bits++;
        value >>= 1;
    }
    return bits;
}

/**
 * @brief 判断电流值是否在 [Imin, Imax] 范围内
 *
 * @param[in] current 待检测电流值
 * @param[in] Imin    范围下限
 * @param[in] Imax    范围上限
 * @retval 1 在范围内
 * @retval 0 超出范围
 */
static uint8_t CurIsInSide(uint16_t current, uint16_t Imin, uint16_t Imax)
{
    if (current < Imin) return 0;
    if (Imax < current) return 0;
    return 1;
}

/**
 * @brief 计算电流值到范围边界的距离
 *
 * @param[in] current 待检测电流值
 * @param[in] Imin    范围下限
 * @param[in] Imax    范围上限
 * @return 距离值（在范围内返回 0）
 */
static uint16_t GetDif(uint16_t current, uint16_t Imin, uint16_t Imax)
{
    if (current < Imin) return Imin - current;
    if (Imax < current) return current - Imax;
    return 0;
}

/**
 * @brief 从 OTP 校准数据中查找匹配的 Dref 参考值
 *
 * 遍历 OTP 中的校准分段数据（每段 2 字节，共 TEST_POINT_NUM 段），
 * 根据每段的 Dref 值计算电流范围：
 * - Imin = (seg * 163.84 + 1.0) * Dref / 1000
 * - Imax = ((seg + 1) * 163.84) * Dref / 1000
 *
 * 找到包含目标电流的分段后返回对应的 Dref。
 * 若无精确匹配，选择距离最近的分段。
 *
 * @param[in]  chip_id  芯片编号
 * @param[in]  Channel  通道编号（决定校准基地址）
 * @param[in]  current  目标电流值
 * @param[out] aSeg     匹配到的分段索引
 * @return Dref 参考值
 */
static uint16_t GetDrefIselIseg(uint8_t chip_id, uint8_t Channel,
                                uint16_t current, uint8_t *aSeg)
{
    uint8_t tSeg;
    uint16_t Dref, Dref1 = 0;
    uint32_t Imax, Imin;
    uint32_t tDif = 0xFFFFFFFF, tmp;
    uint8_t dataADC1, dataADC2;

    int start_address_add;

    /* 根据通道选择校准数据基地址 */
    if (Channel == WAVEFORM_GEN_CH0) {
        start_address_add = CALIB_BASE_ADDRESS_0;
    } else {
        start_address_add = CALIB_BASE_ADDRESS_1;
    }

    for (tSeg = 0; tSeg < TEST_POINT_NUM * 2; tSeg += 2) {
        /* 从 OTP 读取校准数据（2 字节 Dref） */
        dataADC1 = nnc6521_spi_otp_read(chip_id, start_address_add + 2 * tSeg);
        dataADC2 = nnc6521_spi_otp_read(chip_id, start_address_add + 2 * tSeg + 1);
        Dref = ((uint16_t)dataADC2 << 8) | (uint16_t)dataADC1;

        /* 计算该分段的电流范围 */
        Imin = ((tSeg / 2) * 163.84f + 1.0f) * Dref;
        Imin /= 1000;
        Imax = (((tSeg / 2) + 1) * 163.84f) * Dref;
        Imax /= 1000;

        if (CurIsInSide(current, Imin, Imax)) {
            /* 精确匹配 */
            *aSeg = tSeg / 2;
            return Dref;
        } else {
            /* 记录距离最近的分段 */
            tmp = GetDif(current, Imin, Imax);
            if (tmp < tDif) {
                tDif = tmp;
                *aSeg = tSeg / 2;
                Dref1 = Dref;
            } else {
                return Dref1;  /* 距离开始增大，返回上一次的最佳匹配 */
            }
        }
    }

    return Dref1;
}

/**
 * @brief 将目标电流值转换为 12-bit DAC 数值
 *
 * 计算公式：Ival = (current * 1000) / Dref，结果限制在 0~4096。
 * Dref 由 OTP 校准数据提供，通过 GetDrefIselIseg() 获取。
 *
 * @param[in] chip_id 芯片编号
 * @param[in] current 目标电流值（单位：mA）
 * @param[in] channel 通道编号
 * @return 12-bit DAC 数值（0~4096）
 */
static uint32_t Current_Output(uint8_t chip_id, uint32_t current, uint8_t channel)
{
    uint8_t tSeg;
    uint16_t Dref;

    uint32_t Ireq = current * 1;
    float Ival_f;
    uint32_t Ival;

    Dref = GetDrefIselIseg(chip_id, channel, current, &tSeg);
    Ival_f = (float)(((float)Ireq * 1000) / ((float)Dref));
    Ival = (uint32_t)(Ival_f + 0.5f);
    Ival = (4096 < Ival) ? 4096 : Ival;  /* 限制最大值 */
    return Ival;
}
