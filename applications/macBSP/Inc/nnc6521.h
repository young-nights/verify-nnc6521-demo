/**
  ******************************************************************************
  * @file    nnc6521.h
  * @brief   NNC6521 双芯片驱动头文件（RT-Thread + STM32F103 平台）
  *          支持软件 SPI（GPIO 位操作），每颗芯片独立引脚映射。
  ******************************************************************************
  */

#ifndef __NNC6521_H__
#define __NNC6521_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "nnc6521_reg.h"
#include "stm32f1xx_hal.h"

/* 芯片编号定义 --------------------------------------------------------------*/
#define NNC6521_CHIP_1      0   /**< 芯片 1 编号 */
#define NNC6521_CHIP_2      1   /**< 芯片 2 编号 */
#define NNC6521_NUM_CHIPS   2   /**< 系统中 NNC6521 芯片总数 */

/**
 * 芯片 1 引脚映射：MOSI=PA7, CSN=PA4, SCLK=PA5, MISO=PA6, CHIP_EN=PC5, INTB=PC4
 * 芯片 2 引脚映射：MOSI=PB15, CSN=PB12, SCLK=PB13, MISO=PB14, CHIP_EN=PC7, INTB=PC6
 */

/**
 * @brief NNC6521 引脚映射结构体
 *
 * 每颗 NNC6521 芯片的 SPI 引脚和控制引脚映射，用于软件 SPI 位操作。
 * 通过该结构体可为不同芯片配置独立的 GPIO 引脚。
 */
typedef struct {
    /* SPI 引脚 */
    GPIO_TypeDef *mosi_port;    /**< MOSI 端口（主出从入），如 GPIOA */
    uint16_t      mosi_pin;     /**< MOSI 引脚号，如 GPIO_PIN_7 */
    GPIO_TypeDef *csn_port;     /**< CSN 端口（片选信号，低电平有效），如 GPIOA */
    uint16_t      csn_pin;      /**< CSN 引脚号，如 GPIO_PIN_4 */
    GPIO_TypeDef *sclk_port;    /**< SCLK 端口（SPI 时钟），如 GPIOA */
    uint16_t      sclk_pin;     /**< SCLK 引脚号，如 GPIO_PIN_5 */
    GPIO_TypeDef *miso_port;    /**< MISO 端口（主入从出），如 GPIOA */
    uint16_t      miso_pin;     /**< MISO 引脚号，如 GPIO_PIN_6 */
    /* 控制引脚 */
    GPIO_TypeDef *chip_en_port; /**< CHIP_EN 端口（芯片使能，高电平有效），如 GPIOC */
    uint16_t      chip_en_pin;  /**< CHIP_EN 引脚号，如 GPIO_PIN_5 */
    GPIO_TypeDef *intb_port;    /**< INTB 端口中断输出（低电平有效），如 GPIOC */
    uint16_t      intb_pin;     /**< INTB 引脚号，如 GPIO_PIN_4 */
} nnc6521_pin_map_t;

/* 波形数据声明 --------------------------------------------------------------*/
extern float normalized_sine_waveform_16[16];       /**< 16 点归一化正弦波形 */
extern float normalized_sine_waveform_64[64];       /**< 64 点归一化正弦波形（完整） */
extern float normalized_sine_waveform_64_1[32];     /**< 64 点正弦波形前半段（0→峰值） */
extern float normalized_sine_waveform_64_2[32];     /**< 64 点正弦波形后半段（峰值→0） */
extern float normalized_sine_waveform_128[128];     /**< 128 点归一化正弦波形（完整） */
extern float normalized_sine_waveform_128_1[64];    /**< 128 点正弦波形前半段 */
extern float normalized_sine_waveform_128_2[64];    /**< 128 点正弦波形后半段 */
extern float normalized_triangle_waveform_64[64];   /**< 64 点归一化三角波形（完整） */
extern float normalized_triangle_waveform_64_1[32]; /**< 64 点三角波形前半段 */
extern float normalized_triangle_waveform_64_2[32]; /**< 64 点三角波形后半段 */
extern float normalized_triangle_waveform_128[128]; /**< 128 点归一化三角波形（完整） */
extern float normalized_triangle_waveform_128_1[64];/**< 128 点三角波形前半段 */
extern float normalized_triangle_waveform_128_2[64];/**< 128 点三角波形后半段 */
extern float normalized_pulse_waveform_128[128];    /**< 128 点归一化脉冲波形 */
extern float normalized_user_waveform_128[128];     /**< 128 点自定义用户波形（指数衰减包络） */

/* ============================================================================
 *  GPIO 初始化
 * ===========================================================================*/

/**
 * @brief 初始化两颗 NNC6521 芯片的所有 GPIO 引脚
 *
 * 配置 MOSI/CSN/SCLK/CHIP_EN 为推挽输出模式，MISO/INTB 为浮空输入模式。
 * 同时使能对应 GPIO 端口时钟。调用此函数后，芯片即可通过 SPI 进行通信。
 *
 * @note 该函数必须在任何 SPI 读写操作之前调用
 * @note 芯片 1 使用 GPIOA + GPIOC，芯片 2 使用 GPIOB + GPIOC
 *
 * @code
 * // 初始化所有 NNC6521 GPIO
 * nnc6521_gpio_init();
 * @endcode
 *
 * @see nnc6521_init
 */
void nnc6521_gpio_init(void);

/* ============================================================================
 *  SPI 层（软件位操作）
 * ===========================================================================*/

/**
 * @brief 通过软件 SPI 向 NNC6521 寄存器写入一个字节
 *
 * 写入协议为 4 字节时序：[addr, cmd, 0x11, data]
 * - 字节 0：寄存器地址
 * - 字节 1：命令字（普通寄存器 cmd=0x80，波形寄存器 cmd=0xC0）
 * - 字节 2：数据标记（固定为 0x11）
 * - 字节 3：写入数据
 *
 * SPI 模式：CPOL=0, CPHA=0, MSB 先传。
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     寄存器地址
 * @param[in] data     要写入的数据字节
 * @param[in] is_wave  1=波形寄存器（cmd 0xC0），0=普通寄存器（cmd 0x80）
 *
 * @note CSN 信号在传输前后自动拉高/拉低
 *
 * @code
 * // 向芯片 1 的普通寄存器 0x01 写入 0x00
 * nnc6521_spi_write(NNC6521_CHIP_1, 0x01, 0x00, 0);
 *
 * // 向芯片 1 的波形寄存器 0x04 写入 0xFF
 * nnc6521_spi_write(NNC6521_CHIP_1, 0x04, 0xFF, 1);
 * @endcode
 *
 * @see nnc6521_spi_read, nnc6521_write_reg, nnc6521_write_wave_reg
 */
void nnc6521_spi_write(uint8_t chip_id, uint8_t addr, uint8_t data, uint8_t is_wave);

/**
 * @brief 通过软件 SPI 从 NNC6521 寄存器读取一个字节
 *
 * 读取协议为 3 字节时序：[addr, cmd, dummy]，数据在第 3 字节返回。
 * - 字节 0：寄存器地址
 * - 字节 1：命令字（普通寄存器 cmd=0x00，波形寄存器 cmd=0x40）
 * - 字节 2：空操作字节，MISO 上返回实际数据
 *
 * SPI 模式：CPOL=0, CPHA=0, MSB 先传。
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     寄存器地址
 * @param[in] is_wave  1=波形寄存器（cmd 0x40），0=普通寄存器（cmd 0x00）
 *
 * @return 读取到的数据字节
 *
 * @code
 * // 从芯片 1 的普通寄存器 0x01 读取
 * uint8_t val = nnc6521_spi_read(NNC6521_CHIP_1, 0x01, 0);
 * @endcode
 *
 * @see nnc6521_spi_write, nnc6521_read_reg, nnc6521_read_wave_reg
 */
uint8_t nnc6521_spi_read(uint8_t chip_id, uint8_t addr, uint8_t is_wave);

/**
 * @brief 从 OTP 存储器读取一个字节
 *
 * OTP（One-Time Programmable）存储器用于保存出厂校准数据。
 * 读取流程：
 * 1. 向寄存器 0x1D 写入 OTP 地址
 * 2. 向寄存器 0x1B 写入 0x54（OTP 读命令）
 * 3. 从寄存器 0x1E 读取数据
 * 4. 向寄存器 0x1B 写入 0x50（结束 OTP 读取）
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 * @param[in] addr     OTP 地址
 *
 * @return OTP 数据字节
 *
 * @note OTP 存储器为一次性可编程，只能读取不能写入
 * @note 校准数据通常存储在地址 0x00~0x63 范围内
 *
 * @code
 * // 读取芯片 1 的 OTP 地址 0x00 处的校准数据
 * uint8_t cal = nnc6521_spi_otp_read(NNC6521_CHIP_1, 0x00);
 * @endcode
 *
 * @see nnc6521_spi_read
 */
uint8_t nnc6521_spi_otp_read(uint8_t chip_id, uint8_t addr);

/* 便捷宏：简化普通/波形寄存器的读写调用 */
#define nnc6521_write_reg(cid, addr, data)      nnc6521_spi_write((cid), (addr), (data), 0)   /**< 写普通寄存器 */
#define nnc6521_read_reg(cid, addr)             nnc6521_spi_read((cid), (addr), 0)            /**< 读普通寄存器 */
#define nnc6521_write_wave_reg(cid, addr, data) nnc6521_spi_write((cid), (addr), (data), 1)   /**< 写波形寄存器 */
#define nnc6521_read_wave_reg(cid, addr)        nnc6521_spi_read((cid), (addr), 1)            /**< 读波形寄存器 */

/* ============================================================================
 *  NNC6521 高级 API
 * ===========================================================================*/

/**
 * @brief NNC6521 芯片上电初始化
 *
 * 执行完整的上电初始化序列：
 * 1. 拉低 CHIP_EN → 延时 → 拉高 CHIP_EN → 延时（硬件复位）
 * 2. 写 PMU 寄存器 0x00（复位波形发生器）
 * 3. 配置时钟分频（PCLK div=1，2 MHz）
 * 4. 使能全局驱动
 *
 * @param[in] chip_id  芯片编号，取值 NNC6521_CHIP_1 或 NNC6521_CHIP_2
 *
 * @note 必须在 nnc6521_gpio_init() 之后调用
 * @note 初始化完成后，芯片处于待机状态，需要调用波形输出函数才能产生刺激信号
 *
 * @code
 * nnc6521_gpio_init();
 * nnc6521_init(NNC6521_CHIP_1);
 * nnc6521_init(NNC6521_CHIP_2);
 * @endcode
 *
 * @see nnc6521_gpio_init, nnc6521_preloaded_waveform
 */
void nnc6521_init(uint8_t chip_id);

/**
 * @brief 输出 NNC6521 内置预加载波形（正弦/脉冲/三角波）
 *
 * 使用芯片内部 ROM 中预存的波形数据，无需外部 SPI 传输波形点数据。
 * 适用于标准波形输出，功耗低、响应快。
 *
 * @param[in] chip_id               芯片编号
 * @param[in] u8_Channel            波形通道，WAVEFORM_GEN_CH0 或 WAVEFORM_GEN_CH1
 * @param[in] u8_Waveform           波形类型：0=正弦, 1=脉冲, 2=三角波
 * @param[in] u8_PointNum           每周期采样点数（16/32/64/128）
 * @param[in] u8_CI                 驱动电流档位索引（0~7），值越大驱动能力越强
 * @param[in] u32_Positive_Interval 正半波时钟周期数（控制正半波持续时间）
 * @param[in] u32_Negative_Interval 负半波时钟周期数（控制负半波持续时间）
 * @param[in] u32_Silent_Time       静默时间（时钟周期数，波形输出后的等待间隔）
 * @param[in] u16_Rest_Time         休息时间（时钟周期数，死区时间）
 *
 * @note 正/负半波间隔决定了波形频率：freq = PCLK / (Positive_Interval + Negative_Interval)
 * @note 调用前需确保已完成 nnc6521_init() 初始化
 *
 * @code
 * // 输出 64 点正弦波，50 Hz，CI=4
 * nnc6521_preloaded_waveform(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
 *                            WAVEFORM_SINE, 64, 4,
 *                            20000, 20000, 600, 0);
 * @endcode
 *
 * @see nnc6521_customized_waveform, nnc6521_amplitude_modulation
 */
void nnc6521_preloaded_waveform(uint8_t chip_id,
                                uint8_t u8_Channel,
                                uint8_t u8_Waveform,
                                uint8_t u8_PointNum,
                                uint8_t u8_CI,
                                uint16_t u32_Positive_Interval,
                                uint16_t u32_Negative_Interval,
                                uint32_t u32_Silent_Time,
                                uint16_t u16_Rest_Time);

/**
 * @brief 输出自定义 SPI 波形（通过归一化数组 + 电流校准）
 *
 * 将归一化波形数据（0.0~1.0）通过 SPI 逐点写入 NNC6521 波形存储器，
 * 驱动芯片按照自定义形状输出电流波形。内部会自动进行电流校准，
 * 将归一化值转换为实际 DAC 码值。
 *
 * @param[in] chip_id                  芯片编号
 * @param[in] u8_Channel               波形通道
 * @param[in] u8_PointNum              每周期采样点数
 * @param[in] f_Normalized_array       归一化波形数组指针（范围 0.0~1.0）
 * @param[in] u32_Max_current          最大输出电流（mA），实际电流由校准数据决定
 * @param[in] u32_Positive_Interval    正半波时钟周期数
 * @param[in] u32_Negative_Interval    负半波时钟周期数
 * @param[in] u32_Silent_Time          静默时间（时钟周期数）
 * @param[in] u16_Rest_Time            休息时间（时钟周期数）
 * @param[in] u8_Asymmetric_Symmetric  0=非对称波形（使用全部点数），1=对称波形（使用半数点+镜像）
 *
 * @note 对称模式下，PointNum 需为偶数，实际写入 PointNum/2 个点
 * @note 波形数据会被校准后写入芯片内部波形 RAM
 *
 * @code
 * nnc6521_customized_waveform(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
 *                             128, normalized_sine_waveform_128,
 *                             50, 20000, 20000, 0, 0, 0);
 * @endcode
 *
 * @see nnc6521_preloaded_waveform, nnc6521_wavegen_config
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
                                 uint8_t u8_Asymmetric_Symmetric);

/**
 * @brief 幅度调制波形输出（包络 + 载波）
 *
 * 使用归一化包络数组控制载波幅度，实现幅度调制（AM）效果。
 * 适用于需要低频包络调制高频载波的场景（如美容仪的"循环塑形"模式）。
 *
 * @param[in] chip_id                      芯片编号
 * @param[in] u8_Channel                   波形通道
 * @param[in] u8_PointNum                  包络波形每周期采样点数
 * @param[in] f_Normalized_Envelope_array  归一化包络数组指针（范围 0.0~1.0）
 * @param[in] u32_Max_current              最大输出电流（mA）
 * @param[in] u16_Carrier                  载波半波时钟周期数（控制载波频率）
 * @param[in] u32_Silent_Time              静默时间（时钟周期数）
 * @param[in] u16_Interval                 包络间隔时钟周期数（控制包络频率）
 *
 * @note 载波频率 = PCLK / (2 * carrier_clk)
 * @note 包络频率 = PCLK / (point_num * am_interval)
 *
 * @code
 * // 4 kHz 载波，10 Hz 包络，64 点正弦包络
 * nnc6521_amplitude_modulation(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
 *                              64, normalized_sine_waveform_64,
 *                              50, 250, 0, 3125);
 * @endcode
 *
 * @see nnc6521_customized_waveform
 */
void nnc6521_amplitude_modulation(uint8_t chip_id,
                                  uint8_t u8_Channel,
                                  uint8_t u8_PointNum,
                                  float *f_Normalized_Envelope_array,
                                  uint32_t u32_Max_current,
                                  uint16_t u16_Carrier,
                                  uint32_t u32_Silent_Time,
                                  uint16_t u16_Interval);

/**
 * @brief 初始化导联脱落检测（LOD, Lead-Off Detection）
 *
 * 配置 LOD 模块的阈值、延迟和目标参数。当电极与皮肤接触不良时，
 * LOD 模块可通过比较器检测到阻抗变化并触发中断。
 *
 * @param[in] chip_id           芯片编号
 * @param[in] u16_LOD_Threshold LOD 比较器阈值（16-bit）
 * @param[in] u32_LOD_DLY       LOD 延迟计数（32-bit）
 * @param[in] u8_LOD_TGT        LOD 目标值
 *
 * @note LOD 初始化后默认使能 CH1 和 CH2 两个通道的 VDAC 和比较器
 * @note 调用后会自动清除 LOD 中断标志
 *
 * @code
 * nnc6521_lod_init(NNC6521_CHIP_1, 0x00F0, 0x00000000, 0x10);
 * @endcode
 *
 * @see nnc6521_scd_init, nnc6521_clear_lod_int
 */
void nnc6521_lod_init(uint8_t chip_id,
                      uint16_t u16_LOD_Threshold,
                      uint32_t u32_LOD_DLY,
                      uint8_t u8_LOD_TGT);

/**
 * @brief 初始化短路检测（SCD, Short-Circuit Detection）
 *
 * 配置 SCD 模块的 DAC 阈值、比较地址和中断参数。
 * 当输出端发生短路时，SCD 模块可检测到异常电流并触发中断，
 * 同时可自动停止波形输出以保护电路。
 *
 * @param[in] chip_id              芯片编号
 * @param[in] u8_CH0_DAC_Threshold CH0 通道 DAC 阈值
 * @param[in] u8_CH0_Addr_0        CH0 比较地址 0
 * @param[in] u8_CH0_Addr_1        CH0 比较地址 1
 * @param[in] u8_CH0_Int_Num       CH0 中断触发点数
 * @param[in] u8_CH1_DAC_Threshold CH1 通道 DAC 阈值
 * @param[in] u8_CH1_Addr_0        CH1 比较地址 0
 * @param[in] u8_CH1_Addr_1        CH1 比较地址 1
 * @param[in] u8_CH1_Int_Num       CH1 中断触发点数
 *
 * @note SCD 初始化后默认使能两个通道的 VDAC 和刺激比较器
 * @note 调用后会自动清除 SCD 中断标志
 *
 * @code
 * nnc6521_scd_init(NNC6521_CHIP_1,
 *                  0x04, 0x00, 0x10, 0x01,
 *                  0x04, 0x20, 0x30, 0x01);
 * @endcode
 *
 * @see nnc6521_lod_init, nnc6521_clear_scd_int
 */
void nnc6521_scd_init(uint8_t chip_id,
                      uint8_t u8_CH0_DAC_Threshold,
                      uint8_t u8_CH0_Addr_0,
                      uint8_t u8_CH0_Addr_1,
                      uint8_t u8_CH0_Int_Num,
                      uint8_t u8_CH1_DAC_Threshold,
                      uint8_t u8_CH1_Addr_0,
                      uint8_t u8_CH1_Addr_1,
                      uint8_t u8_CH1_Int_Num);

/**
 * @brief 使能或禁用指定通道的波形发生器（AWG）
 *
 * 通过修改波形控制寄存器的 enable_wavegen 位来控制 AWG 的开关。
 * 在切换波形前应先禁用 AWG，配置完成后再使能。
 *
 * @param[in] chip_id         芯片编号
 * @param[in] AWG_ChannelNum  波形通道号（WAVEFORM_GEN_CH0 或 WAVEFORM_GEN_CH1）
 * @param[in] Enable_Disable  1=使能，0=禁用
 *
 * @code
 * // 禁用通道 0
 * nnc6521_awg_enable_disable(NNC6521_CHIP_1, WAVEFORM_GEN_CH0, 0);
 * // 使能通道 0
 * nnc6521_awg_enable_disable(NNC6521_CHIP_1, WAVEFORM_GEN_CH0, 1);
 * @endcode
 *
 * @see nnc6521_wavegen_config
 */
void nnc6521_awg_enable_disable(uint8_t chip_id,
                                uint8_t AWG_ChannelNum,
                                uint8_t Enable_Disable);

/**
 * @brief 清除导联脱落检测（LOD）中断标志
 *
 * 通过读取 LEAD_OFF_INT 寄存器并翻转 status_clear 位来清除中断。
 *
 * @param[in] chip_id  芯片编号
 *
 * @see nnc6521_lod_init
 */
void nnc6521_clear_lod_int(uint8_t chip_id);

/**
 * @brief 清除短路检测（SCD）中断标志
 *
 * 向 ANA_INTR_SIM_CL 寄存器写入 0x03 来清除 CH1 和 CH2 的中断标志。
 *
 * @param[in] chip_id  芯片编号
 *
 * @see nnc6521_scd_init
 */
void nnc6521_clear_scd_int(uint8_t chip_id);

/**
 * @brief Enable analog output stage (VDAC + driver amplifier) for a channel.
 *
 * Must be called before waveform output to enable the analog frontend.
 * Without this, only a digital square wave appears at the output pin.
 *
 * @param[in] chip_id  NNC6521_CHIP_1 or NNC6521_CHIP_2
 * @param[in] channel  WAVEFORM_GEN_CH0 or WAVEFORM_GEN_CH1
 */
void nnc6521_analog_enable(uint8_t chip_id, uint8_t channel);

/**
 * @brief 配置波形发生器寄存器（核心配置函数）
 *
 * 根据 waveform_TypeDef 结构体中的参数，完整配置波形发生器的所有寄存器，
 * 包括：采样点数、时序参数、驱动电流、中断配置等。
 * 对于 SPI 自定义波形（WAVEFORM_SPI），还会执行电流校准并将波形数据逐点写入。
 *
 * 配置流程：
 * 1. 禁用 AWG
 * 2. 使能模拟通道
 * 3. 写入点数、配置、时序等寄存器
 * 4. 若为 SPI 波形：校准电流 → 写入波形数据
 * 5. 配置交替模式寄存器（如需要）
 * 6. 使能 AWG
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] wf                        波形配置结构体指针
 * @param[in] normalized_waveform_array 归一化波形数组（SPI 模式使用，预加载模式传 NULL）
 * @param[in] max_current               最大输出电流（mA）
 *
 * @note 该函数是大多数波形输出函数的底层实现
 * @note 调用前需先设置好 wf 结构体的所有字段
 *
 * @see nnc6521_preloaded_waveform, nnc6521_customized_waveform
 */
void nnc6521_wavegen_config(uint8_t chip_id,
                            waveform_TypeDef *wf,
                            float *normalized_waveform_array,
                            uint32_t max_current);

/**
 * @brief 更新波形幅度（用于中断上下文中动态调整）
 *
 * 在波形输出过程中，通过重新校准并写入波形数据来实时调整幅度。
 * 适用于需要动态改变刺激强度的场景。
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] Channel                   波形通道
 * @param[in] u8_PointNum              采样点数
 * @param[in] normalized_waveform_array 归一化波形数组
 * @param[in] max_current               最大输出电流（mA）
 *
 * @note 该函数会重新计算校准值并逐点写入波形 RAM
 * @note 适合在定时器中断或 DMA 回调中调用
 *
 * @see nnc6521_customized_amplitude_addr
 */
void nnc6521_customized_amplitude(uint8_t chip_id,
                                  uint8_t Channel,
                                  uint8_t u8_PointNum,
                                  float *normalized_waveform_array,
                                  uint32_t max_current);

/**
 * @brief 更新指定地址范围内的波形幅度
 *
 * 与 nnc6521_customized_amplitude 类似，但只更新指定地址范围内的波形数据，
 * 而非整个波形 RAM。适用于需要局部更新波形的场景（如中断回调中更新部分周期）。
 *
 * @param[in] chip_id                   芯片编号
 * @param[in] Channel                   波形通道
 * @param[in] u8_Start_Address          起始地址（包含）
 * @param[in] u8_End_Address            结束地址（不包含）
 * @param[in] normalized_waveform_array 归一化波形数组
 * @param[in] max_current               最大输出电流（mA）
 *
 * @note 函数被放置在 .nnc6521_amfunc 段，可在 RAM 中执行以获得更快的响应速度
 * @note 地址范围为 [u8_Start_Address, u8_End_Address)
 *
 * @see nnc6521_customized_amplitude
 */
__attribute__((section(".nnc6521_amfunc")))
void nnc6521_customized_amplitude_addr(uint8_t chip_id,
                                       uint8_t Channel,
                                       uint8_t u8_Start_Address,
                                       uint8_t u8_End_Address,
                                       float *normalized_waveform_array,
                                       uint32_t max_current);

#ifdef __cplusplus
}
#endif

#endif /* __NNC6521_H__ */
