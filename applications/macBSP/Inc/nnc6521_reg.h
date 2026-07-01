/**
  ******************************************************************************
  * @file    nnc6521_reg.h
  * @brief   NNC6521 寄存器定义文件
  *          包含所有寄存器地址、union/struct 类型定义，涵盖 AWG、LOD、SCD 模块。
  ******************************************************************************
  */

#ifndef __NNC6521_REG_H__
#define __NNC6521_REG_H__

#include <stdint.h>

/* 校准相关宏 ----------------------------------------------------------------*/
#define ARRAY_SIZE          16      /**< 自定义波形数组采样点数 */
#define TEST_POINT_NUM      25      /**< 校准测试点数，范围 3~25 */
#define CALIB_BASE_ADDRESS_0 0x00   /**< CH1 校准数据基地址（OTP 起始地址） */
#define CALIB_BASE_ADDRESS_1 0x32   /**< CH2 校准数据基地址（OTP 起始地址） */

/* ============================================================================
 *  NNC6521 通用寄存器地址列表
 * ===========================================================================*/

/* 电源管理与时钟控制 */
#define PMU_REG_ADDR                0x01    /**< 电源管理单元寄存器（使能/休眠/复位） */
#define CLK_CTRL_REG_ADDR           0x02    /**< 时钟控制寄存器（PCLK 分频、内部时钟输出） */
#define WAVEGEN_GLOBAL_REG_0        0x03    /**< 波形发生器全局控制寄存器（全局驱动使能） */

/* GPIO 与比较器输出控制 */
#define GPIO_COMP_OUT_CTRL_ADDR     0x23    /**< GPIO 比较器输出控制寄存器 */

/* 导联脱落检测（LOD）寄存器 */
#define LEAD_OFF_CTRL_ADDR          0x30    /**< LOD 控制寄存器（检测模式、DAC 选择、延迟使能） */
#define LEAD_OFF_THR_H_0_ADDR       0x31    /**< LOD 高阈值低 8 位 */
#define LEAD_OFF_THR_H_1_ADDR       0x32    /**< LOD 高阈值高 4 位 */
#define LEAD_OFF_THR_L_0_ADDR       0x33    /**< LOD 低阈值低 8 位 */
#define LEAD_OFF_THR_L_1_ADDR       0x34    /**< LOD 低阈值高 4 位 */
#define LEAD_OFF_DLY_TGT_0_ADDR     0x35    /**< LOD 延迟目标字节 0（最低字节） */
#define LEAD_OFF_DLY_TGT_1_ADDR     0x36    /**< LOD 延迟目标字节 1 */
#define LEAD_OFF_DLY_TGT_2_ADDR     0x37    /**< LOD 延迟目标字节 2 */
#define LEAD_OFF_DLY_TGT_3_ADDR     0x38    /**< LOD 延迟目标字节 3（最高字节） */
#define LEAD_OFF_TGT_ADDR           0x39    /**< LOD 目标值寄存器 */
#define LEAD_OFF_INT_ADDR           0x3A    /**< LOD 中断寄存器（状态/清除/使能） */
#define LEAD_OFF_ANA_ADDR           0x3B    /**< LOD 模拟比较器状态寄存器 */

/* 模拟模块使能寄存器 */
#define ANA_ENABLE_REG_0_ADDR       0x40    /**< 模拟使能寄存器 0（LVD、2 MHz 振荡器） */
#define ANA_ENABLE_REG_1_ADDR       0x41    /**< 模拟使能寄存器 1（CH1 驱动器/比较器/IDAC/VDAC） */
#define ANA_ENABLE_REG_2_ADDR       0x42    /**< 模拟使能寄存器 2（CH2 驱动器/比较器/IDAC/VDAC） */
#define ANA_ENABLE_REG_3_ADDR       0x43    /**< 模拟使能寄存器 3（BIST 自检） */

/* 模拟通用配置寄存器 */
#define ANA_GEN_REG_1_ADDR          0x44    /**< 模拟通用寄存器 1（LVD 电压选择） */
#define ANA_GEN_REG_2_ADDR          0x45    /**< 模拟通用寄存器 2（CH1 VDAC 低 8 位） */
#define ANA_GEN_REG_3_ADDR          0x46    /**< 模拟通用寄存器 3（CH1 VDAC 高 2 位 + 通道配置） */
#define ANA_GEN_REG_4_ADDR          0x47    /**< 模拟通用寄存器 4（CH2 VDAC 低 8 位） */
#define ANA_GEN_REG_5_ADDR          0x48    /**< 模拟通用寄存器 5（CH2 VDAC 高 2 位 + 通道配置） */

/* 中断控制寄存器 */
#define ANA_INTR_EN_ADDR            0x52    /**< 模拟中断使能寄存器（LVD/比较器中断使能） */
#define ANA_INTR_STS_REG_ADDR       0x53    /**< 模拟中断状态寄存器（只读，中断标志） */
#define ANA_INT_COMP_POL_ADDR       0x54    /**< 比较器中断极性与刺激中断使能 */
#define ANA_INT_STOP_WAVEGEN_ADDR   0x55    /**< 中断触发停止波形发生器控制 */
#define ANA_INT_SIM0_A00_ADDR       0x56    /**< 刺激 0 比较地址 A00 */
#define ANA_INT_SIM0_A01_ADDR       0x57    /**< 刺激 0 比较地址 A01 */
#define ANA_INT_SIM1_A10_ADDR       0x58    /**< 刺激 1 比较地址 A10 */
#define ANA_INT_SIM1_A11_ADDR       0x59    /**< 刺激 1 比较地址 A11 */
#define ANA_INT_CH1_INT_NUMBER_ADDR 0x5A    /**< CH1 中断触发点数 */
#define ANA_INT_SIM2_A20_ADDR       0x5B    /**< 刺激 2 比较地址 A20 */
#define ANA_INT_SIM2_A21_ADDR       0x5C    /**< 刺激 2 比较地址 A21 */
#define ANA_INT_SIM3_A30_ADDR       0x5D    /**< 刺激 3 比较地址 A30 */
#define ANA_INT_SIM3_A31_ADDR       0x5E    /**< 刺激 3 比较地址 A31 */
#define ANA_INT_CH2_INT_NUMBER_ADDR 0x5F    /**< CH2 中断触发点数 */
#define ANA_INTR_SIM_CL_ADDR        0x60    /**< 刺激中断清除寄存器（写 1 清除） */

/* ============================================================================
 *  波形发生器寄存器地址定义
 * ===========================================================================*/

#define WAVEFORM_GEN_CH0                    0x00    /**< 波形发生器通道 0 */
#define WAVEFORM_GEN_CH1                    0x01    /**< 波形发生器通道 1 */

#define WAVEFORM_GEN_BASE                   0x00    /**< 波形发生器寄存器基地址 */
#define WAVEFORM_GEN_CH_OFFSET              0x40    /**< 通道间地址偏移量 */

/* 波形发生器寄存器偏移量（相对于通道基地址） */
#define WG_DRV_CONFIG_REG0_OFFSET           0x00    /**< 驱动配置寄存器 0（休息/负半波/静默/交替模式） */
#define WG_DRV_CTRL_REG0_OFFSET             0x01    /**< 驱动控制寄存器 0（使能/波形选择/对称模式） */
#define WG_DRV_POINT_CONFIG_OFFSET          0x02    /**< 采样点数配置寄存器 */
#define WG_DRV_IN_WAVE_ADDR_OFFSET          0x03    /**< 波形写入地址寄存器（指定写入位置） */
#define WG_DRV_IN_WAVE_OFFSET               0x04    /**< 波形写入数据寄存器（写入采样值） */
#define WG_DRV_REST_CLK_OFFSET              0x05    /**< 休息时钟寄存器（16-bit，死区时间） */
#define WG_DRV_SILENT_CLK_OFFSET            0x07    /**< 静默时钟寄存器（24-bit，波形间隔） */
#define WG_DRV_HALF_WAVE_CLK_PNT_OFFSET     0x0A    /**< 正半波时钟周期寄存器（16-bit） */
#define WG_DRV_NEG_HALF_WAVE_CLK_PNT_OFFSET 0x0C    /**< 负半波时钟周期寄存器（16-bit） */
#define WG_DRV_DELAY_LIM_OFFSET             0x20    /**< 延迟限制寄存器（16-bit） */
#define WG_DRV_NEG_SCALE_OFFSET             0x22    /**< 负半波缩放寄存器（7-bit 缩放值 + 方向） */
#define WG_DRV_NEG_OFFSET_OFFSET            0x23    /**< 负半波偏移寄存器 */
#define WG_DRV_POS_SCALE_OFFSET             0x24    /**< 正半波缩放寄存器（7-bit 缩放值 + 方向） */
#define WG_DRV_POS_OFFSET_OFFSET            0x25    /**< 正半波偏移寄存器 */
#define WG_DRV_SHORT_OFFSET                 0x26    /**< 短路保护寄存器 */
#define WG_DRV_INT_NUM_WAVE_OFFSET          0x27    /**< 中断触发波形点数寄存器 */
#define WG_DRV_INT_REG01_OFFSET             0x28    /**< 中断控制寄存器 01（驱动器编号/中断使能/地址匹配） */
#define WG_DRV_INT_REG02_OFFSET             0x29    /**< 中断控制寄存器 02 */
#define WG_DRV_INT_REG03_OFFSET             0x2A    /**< 中断控制寄存器 03 */
#define WG_DRV_ALT_LIM_OFFSET               0x2B    /**< 交替模式周期限制寄存器（16-bit） */
#define WG_DRV_ALT_SILENT_LIM_OFFSET        0x2D    /**< 交替模式静默间隔寄存器（16-bit） */
#define WG_DRIVE_REG_CTRL2_OFFSET           0x31    /**< 驱动控制寄存器 2（IDAC 高位 + 输出位置） */

/**
 * @brief 计算波形发生器寄存器的绝对地址
 *
 * @param ch          通道号（0 或 1）
 * @param reg_offset  寄存器偏移量
 * @return 寄存器绝对地址
 */
#define WG_REG_ADDR(ch, reg_offset)  (WAVEFORM_GEN_BASE + ((ch) * WAVEFORM_GEN_CH_OFFSET) + (reg_offset))

/* ============================================================================
 *  枚举类型定义
 * ===========================================================================*/

/** @brief PMU 复位模式 */
typedef enum { PMU_NORMAL = 0, PMU_RESET = 1 } pmu_reset_t;

/** @brief PMU 使能/禁用 */
typedef enum { PMU_ENABLE = 0, PMU_DISABLE = 1 } pmu_en_dis_t;

/** @brief PCLK 分频系数 */
typedef enum
{
    PCLK_DIV_1 = 0,     /**< 不分频 */
    PCLK_DIV_2,         /**< 2 分频 */
    PCLK_DIV_4,         /**< 4 分频 */
    PCLK_DIV_8,         /**< 8 分频 */
    PCLK_DIV_16,        /**< 16 分频 */
    PCLK_DIV_32,        /**< 32 分频 */
    PCLK_DIV_64,        /**< 64 分频 */
    PCLK_DIV_128        /**< 128 分频 */
} pclk_div_t;

/** @brief CH1 刺激比较器选择 */
typedef enum {
    STIMU_COMP_SEL_CH1_STIMU0 = 0,  /**< 选择刺激 0 */
    STIMU_COMP_SEL_CH1_STIMU1 = 1,  /**< 选择刺激 1 */
} D2A_STIMU_COMP_SEL_CH1_e;

/** @brief CH2 刺激比较器选择 */
typedef enum {
    STIMU_COMP_SEL_CH2_STIMU2 = 0,  /**< 选择刺激 2 */
    STIMU_COMP_SEL_CH2_STIMU3 = 1,  /**< 选择刺激 3 */
} D2A_STIMU_COMP_SEL_CH2_e;

/** @brief LVD（低压检测）电压选择 */
typedef enum {
    LVD_SEL_4_2V = 0x00,    /**< 4.2V 阈值 */
    LVD_SEL_3_9V = 0x01,    /**< 3.9V 阈值 */
    LVD_SEL_3_6V = 0x02,    /**< 3.6V 阈值 */
    LVD_SEL_3_3V = 0x03,    /**< 3.3V 阈值 */
    LVD_SEL_3_0V = 0x04,    /**< 3.0V 阈值 */
} LVD_SEL_e;

/** @brief DAC 通道选择（用于 LOD） */
typedef enum {
    LO_CH0 = 0x01,      /**< 仅 CH0 */
    LO_CH1 = 0x02,      /**< 仅 CH1 */
    LO_ALL = 0x03,      /**< CH0 + CH1 */
    LO_DISABLE = 0,     /**< 禁用 */
} dac_sel_e;

/* ============================================================================
 *  PMU 控制寄存器类型
 * ===========================================================================*/

/**
 * @brief PMU 寄存器（0x01）位域定义
 *
 * 控制芯片电源模式、波形发生器复位、LOD 复位等。
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t pmuenable       : 1;    /**< PMU 使能位 */
        uint8_t sleepdeep       : 1;    /**< 深度休眠模式 */
        uint8_t hresetreq       : 1;    /**< 硬件复位请求 */
        uint8_t otp_dpstb_en    : 1;    /**< OTP 深度待机使能 */
        uint8_t wave_gen_disable: 1;    /**< 波形发生器禁用 */
        uint8_t wave_gen_rst    : 1;    /**< 波形发生器复位 */
        uint8_t lead_off_dis    : 1;    /**< LOD 禁用 */
        uint8_t lead_off_rst    : 1;    /**< LOD 复位 */
    } bits;
} PMU_REG_t;

/**
 * @brief 时钟控制寄存器（0x02）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t pclk_div     : 3;      /**< PCLK 分频系数（0~7 对应 1~128 分频） */
        uint8_t int_clk_out  : 1;      /**< 内部时钟输出使能 */
        uint8_t reserved_4_7 : 4;      /**< 保留位 */
    } bits;
} CLK_CTRL_REG_t;

/**
 * @brief 波形发生器全局控制寄存器（0x03）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t global_drive_en : 1;    /**< 全局驱动使能 */
        uint8_t reserved_1_7    : 7;    /**< 保留位 */
    } bits;
} WAVEGEN_GLOBAL_REG_0_t;

/* ============================================================================
 *  LOD（导联脱落检测）寄存器类型
 * ===========================================================================*/

/**
 * @brief LOD 控制寄存器（0x30）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t dly_dis          : 1;   /**< 延迟禁用 */
        uint8_t compare_reverse  : 1;   /**< 比较器反转 */
        uint8_t check_mode       : 2;   /**< 检测模式（0=低电平, 1=高电平, 2=窗口） */
        uint8_t dac_sel          : 2;   /**< DAC 通道选择（dac_sel_e） */
        uint8_t reserved_6_7     : 2;   /**< 保留位 */
    } bits;
} LEAD_OFF_CTRL_t;

/** @brief LOD 高阈值低 8 位寄存器（0x31） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_th_h : 8;     /**< 高阈值 [7:0] */
    } bits;
} LEAD_OFF_THR_H_0_t;

/** @brief LOD 高阈值高 4 位寄存器（0x32） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_th_h : 4;     /**< 高阈值 [11:8] */
        uint8_t reserved      : 4;     /**< 保留位 */
    } bits;
} LEAD_OFF_THR_H_1_t;

/** @brief LOD 低阈值低 8 位寄存器（0x33） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_th_l : 8;     /**< 低阈值 [7:0] */
    } bits;
} LEAD_OFF_THR_L_0_t, LEAD_OFF_THR_L_1_t;

/** @brief LOD 延迟目标寄存器（0x35~0x38） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_dly_tgt : 8;  /**< 延迟目标值 */
    } bits;
} LEAD_OFF_DLY_TGT_t;

/** @brief LOD 目标值寄存器（0x39） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_tgt : 8;      /**< 目标值 */
    } bits;
} LEAD_OFF_TGT_t;

/**
 * @brief LOD 中断寄存器（0x3A）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t lead_off_result       : 1;  /**< LOD 检测结果（1=脱落） */
        uint8_t lead_off_status_clear : 1;  /**< 状态清除位（写 1 清除） */
        uint8_t reserved              : 2;  /**< 保留位 */
        uint8_t lead_off_int_en       : 1;  /**< LOD 中断使能 */
        uint8_t reserved_5_7          : 3;  /**< 保留位 */
    } bits;
} LEAD_OFF_INT_t;

/**
 * @brief LOD 模拟比较器状态寄存器（0x3B）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t A2D_COMP0 : 1;         /**< 比较器 0 输出 */
        uint8_t A2D_COMP1 : 1;         /**< 比较器 1 输出 */
        uint8_t reserved_2_7  : 6;     /**< 保留位 */
    } bits;
} LEAD_OFF_ANA_t;

/* ============================================================================
 *  模拟使能寄存器类型
 * ===========================================================================*/

/**
 * @brief 模拟使能寄存器 0（0x40）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t LVD_EN            : 1; /**< LVD（低压检测）使能 */
        uint8_t OSC2MHZ_EN        : 1; /**< 2 MHz 振荡器使能 */
        uint8_t reserved_2_7      : 6; /**< 保留位 */
    } bits;
} ANA_ENABLE_REG_0_t;

/**
 * @brief 模拟使能寄存器 1（0x41）位域定义 —— CH1 模拟模块使能
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t DRIVERA_AMP_EN_CH1       : 1; /**< CH1 驱动放大器使能 */
        uint8_t DRIVERA_CSAMP_EN_CH1     : 1; /**< CH1 驱动电流感应放大器使能 */
        uint8_t COMP_EN_CH1              : 1; /**< CH1 比较器使能 */
        uint8_t IDAC_EN_CH1              : 1; /**< CH1 IDAC 使能 */
        uint8_t VDAC_EN_CH1              : 1; /**< CH1 VDAC 使能 */
        uint8_t D2A_STIMU_COMP_EN_CH1    : 1; /**< CH1 刺激比较器使能 */
        uint8_t D2A_STIMU_COMP_SEL_CH1   : 1; /**< CH1 刺激比较器选择 */
        uint8_t reserved_7               : 1; /**< 保留位 */
    } bits;
} ANA_ENABLE_REG_1_t;

/**
 * @brief 模拟使能寄存器 2（0x42）位域定义 —— CH2 模拟模块使能
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t DRIVERA_AMP_EN_CH2       : 1; /**< CH2 驱动放大器使能 */
        uint8_t DRIVERA_CSAMP_EN_CH2     : 1; /**< CH2 驱动电流感应放大器使能 */
        uint8_t COMP_EN_CH2              : 1; /**< CH2 比较器使能 */
        uint8_t IDAC_EN_CH2              : 1; /**< CH2 IDAC 使能 */
        uint8_t VDAC_EN_CH2              : 1; /**< CH2 VDAC 使能 */
        uint8_t D2A_STIMU_COMP_EN_CH2    : 1; /**< CH2 刺激比较器使能 */
        uint8_t D2A_STIMU_COMP_SEL_CH2   : 1; /**< CH2 刺激比较器选择 */
        uint8_t reserved_7               : 1; /**< 保留位 */
    } bits;
} ANA_ENABLE_REG_2_t;

/**
 * @brief 模拟使能寄存器 3（0x43）位域定义 —— BIST 自检
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t BIST_EN      : 1;      /**< BIST 使能 */
        uint8_t BIST_SEL     : 4;      /**< BIST 选择 */
        uint8_t reserved_5_7 : 3;      /**< 保留位 */
    } bits;
} ANA_ENABLE_REG_3_t;

/* ============================================================================
 *  模拟通用配置寄存器类型
 * ===========================================================================*/

/** @brief 模拟通用寄存器 1（0x44）—— LVD 电压选择 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t LVD_SEL      : 3;      /**< LVD 电压选择（LVD_SEL_e） */
        uint8_t reserved_3_7 : 5;      /**< 保留位 */
    } bits;
} ANA_GEN_REG_1_t;

/** @brief 模拟通用寄存器 2（0x45）—— CH1 VDAC 低 8 位 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t VDAC_DIN_CH1_LSB : 8;  /**< CH1 VDAC 数据低 8 位 */
    } bits;
} ANA_GEN_REG_2_t;

/**
 * @brief 模拟通用寄存器 3（0x46）—— CH1 VDAC 高位 + 通道配置
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t VDAC_DIN_CH1_MSB     : 2; /**< CH1 VDAC 数据高 2 位 [9:8] */
        uint8_t DRIVERA_CSAMP_CH_CH1 : 1; /**< CH1 电流感应放大器通道选择 */
        uint8_t COMP_ISEL_CH1        : 1; /**< CH1 比较器电流选择 */
        uint8_t D2A_VDAC_AMPCHOP_CH1 : 1; /**< CH1 VDAC 放大器斩波使能 */
        uint8_t reserved_5_7         : 3; /**< 保留位 */
    } bits;
} ANA_GEN_REG_3_t;

/** @brief 模拟通用寄存器 4（0x47）—— CH2 VDAC 低 8 位 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t VDAC_DIN_CH2_LSB : 8;  /**< CH2 VDAC 数据低 8 位 */
    } bits;
} ANA_GEN_REG_4_t;

/**
 * @brief 模拟通用寄存器 5（0x48）—— CH2 VDAC 高位 + 通道配置
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t VDAC_DIN_CH2_MSB     : 2; /**< CH2 VDAC 数据高 2 位 [9:8] */
        uint8_t DRIVERA_CSAMP_CH_CH2 : 1; /**< CH2 电流感应放大器通道选择 */
        uint8_t COMP_ISEL_CH2        : 1; /**< CH2 比较器电流选择 */
        uint8_t D2A_VDAC_AMPCHOP_CH2 : 1; /**< CH2 VDAC 放大器斩波使能 */
        uint8_t reserved_5_7         : 3; /**< 保留位 */
    } bits;
} ANA_GEN_REG_5_t;

/* ============================================================================
 *  中断控制寄存器类型
 * ===========================================================================*/

/**
 * @brief 模拟中断使能寄存器（0x52）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_LVD_INTR_EN                : 1; /**< LVD 中断使能 */
        uint8_t ANA_COMP_CH1_INTR_EN           : 1; /**< CH1 比较器中断使能 */
        uint8_t ANA_COMP_CH2_INTR_EN           : 1; /**< CH2 比较器中断使能 */
        uint8_t ANA_COMP_CH1_INTR_TRANS_SEL    : 1; /**< CH1 比较器中断触发沿选择 */
        uint8_t ANA_COMP_CH12_INTR_TRANS_SEL   : 1; /**< CH1/CH2 比较器中断触发沿选择 */
        uint8_t reserved_5_7                   : 3; /**< 保留位 */
    } bits;
} ANA_INTR_EN_t;

/**
 * @brief 模拟中断状态寄存器（0x53）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_LVD_INTR_STS        : 1; /**< LVD 中断状态 */
        uint8_t ANA_COMP_CH1_INTR_STS   : 1; /**< CH1 比较器中断状态 */
        uint8_t ANA_COMP_CH2_INTR_STS   : 1; /**< CH2 比较器中断状态 */
        uint8_t reserved_3_7            : 5; /**< 保留位 */
    } bits;
} ANA_INTR_STS_REG_t;

/**
 * @brief 比较器中断极性寄存器（0x54）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ana_int_comp_pol_reg_0  : 1; /**< 比较器 0 中断极性 */
        uint8_t ANA_STIMU_CH1_INTR_EN   : 1; /**< CH1 刺激中断使能 */
        uint8_t ANA_STIMU_CH2_INTR_EN   : 1; /**< CH2 刺激中断使能 */
        uint8_t reserved_3_7            : 5; /**< 保留位 */
    } bits;
} ANA_INT_COMP_POL_t;

/**
 * @brief 中断停止波形发生器寄存器（0x55）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t Ana_int_stop_wavegen_0 : 1; /**< 中断停止波形发生器 0 */
        uint8_t Ana_int_stop_wavegen_1 : 1; /**< 中断停止波形发生器 1 */
        uint8_t reserved_2_7           : 6; /**< 保留位 */
    } bits;
} ANA_INT_STOP_WAVEGEN_t;

/** @brief 刺激 0 比较地址 A00 寄存器（0x56） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM0_A00 : 8;  /**< 刺激 0 比较地址 A00 */
    } bits;
} ANA_INT_SIM0_A00_t;

/** @brief 刺激 0 比较地址 A01 寄存器（0x57） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM0_A01 : 8;  /**< 刺激 0 比较地址 A01 */
    } bits;
} ANA_INT_SIM0_A01_t;

/** @brief 刺激 1 比较地址 A10 寄存器（0x58） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM1_A10 : 8;  /**< 刺激 1 比较地址 A10 */
    } bits;
} ANA_INT_SIM1_A10_t;

/** @brief 刺激 1 比较地址 A11 寄存器（0x59） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM1_A11 : 8;  /**< 刺激 1 比较地址 A11 */
    } bits;
} ANA_INT_SIM1_A11_t;

/** @brief CH1 中断触发点数寄存器（0x5A） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_CH1_INT_NUMBER : 8; /**< CH1 中断触发波形点数 */
    } bits;
} ANA_INT_CH1_INT_NUMBER_t;

/** @brief 刺激 2 比较地址 A20 寄存器（0x5B） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM2_A20 : 8;  /**< 刺激 2 比较地址 A20 */
    } bits;
} ANA_INT_SIM2_A20_t;

/** @brief 刺激 2 比较地址 A21 寄存器（0x5C） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM2_A21 : 8;  /**< 刺激 2 比较地址 A21 */
    } bits;
} ANA_INT_SIM2_A21_t;

/** @brief 刺激 3 比较地址 A30 寄存器（0x5D） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM3_A30 : 8;  /**< 刺激 3 比较地址 A30 */
    } bits;
} ANA_INT_SIM3_A30_t;

/** @brief 刺激 3 比较地址 A31 寄存器（0x5E） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_SIM3_A31 : 8;  /**< 刺激 3 比较地址 A31 */
    } bits;
} ANA_INT_SIM3_A31_t;

/** @brief CH2 中断触发点数寄存器（0x5F） */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_INT_CH2_INT_NUMBER : 8; /**< CH2 中断触发波形点数 */
    } bits;
} ANA_INT_CH2_INT_NUMBER_t;

/**
 * @brief 刺激中断清除寄存器（0x60）位域定义
 */
typedef union {
    uint8_t all;                    /**< 整字节访问 */
    struct {
        uint8_t ANA_STIMU_CH1_INTR_STS : 1; /**< CH1 刺激中断状态（写 1 清除） */
        uint8_t ANA_STIMU_CH2_INTR_STS : 1; /**< CH2 刺激中断状态（写 1 清除） */
        uint8_t reserved_2_7           : 6; /**< 保留位 */
    } bits;
} ANA_INTR_SIM_CL_t;

/* ============================================================================
 *  波形发生器寄存器类型
 * ===========================================================================*/

/**
 * @brief 波形驱动配置寄存器 0 位域定义
 *
 * 控制波形的休息、负半波、静默、交替、多电极等模式。
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t rest_enable        : 1; /**< 休息模式使能（正负半波间插入死区） */
        uint8_t negative_enable    : 1; /**< 负半波使能（对称波形输出） */
        uint8_t silent_enable      : 1; /**< 静默模式使能（波形周期间插入静默间隔） */
        uint8_t sourceB_enable     : 1; /**< Source B 使能 */
        uint8_t alternating_pos    : 1; /**< 交替正半波模式（用于幅度调制） */
        uint8_t continue_repeat    : 1; /**< 连续重复模式 */
        uint8_t multi_electrode    : 1; /**< 多电极模式 */
        uint8_t disable_positive   : 1; /**< 禁用正半波 */
    } bits;
} WG_DrvConfigReg0_t;

/** @brief 波形类型选择枚举 */
typedef enum {
    WAVEFORM_SINE        = 0x00,    /**< 正弦波 */
    WAVEFORM_PULSE       = 0x01,    /**< 脉冲波 */
    WAVEFORM_TRIANGLE    = 0x02,    /**< 三角波 */
    WAVEFORM_SPI         = 0x03     /**< SPI 自定义波形 */
} WaveformSelect_t;

/**
 * @brief 波形驱动控制寄存器 0 位域定义
 *
 * 控制波形发生器使能、波形类型选择、预加载模式、对称模式等。
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t enable_wavegen     : 1; /**< 波形发生器使能 */
        uint8_t waveform_select    : 2; /**< 波形类型选择（WaveformSelect_t） */
        uint8_t num_waveforms      : 3; /**< 波形数量 */
        uint8_t preload_mode       : 1; /**< 预加载模式 */
        uint8_t symmetric_wave     : 1; /**< 对称波形模式（使用半数点+镜像） */
    } bits;
} WG_DrvCtrlReg0_t;

/** @brief 波形采样点数配置寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvPointConfig_t;

/** @brief 波形写入地址寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvInWaveAddrReg_t;

/** @brief 波形写入数据寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvInWaveReg_t;

/** @brief 休息时钟寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvRestClkReg_t;

/** @brief 静默时钟寄存器（24-bit，存储为 32-bit） */
typedef union {
    uint32_t value;                 /**< 整字访问 */
} WG_DrvSilentClkReg_t;

/** @brief 正半波时钟周期寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvHalfWaveClkPntReg_t;

/** @brief 负半波时钟周期寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvNegHalfWaveClkPntReg_t;

/** @brief 延迟限制寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvDelayLimReg_t;

/**
 * @brief 负半波缩放寄存器位域定义
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t scale_value : 7;        /**< 缩放值（0~127） */
        uint8_t scale_dir   : 1;        /**< 缩放方向（0=缩小, 1=放大） */
    } bits;
} WG_DrvNegScaleReg_t;

/** @brief 负半波偏移寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvNegOffsetReg_t;

/**
 * @brief 正半波缩放寄存器位域定义
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t scale_value : 7;        /**< 缩放值（0~127） */
        uint8_t scale_dir   : 1;        /**< 缩放方向（0=缩小, 1=放大） */
    } bits;
} WG_DrvPosScaleReg_t;

/** @brief 正半波偏移寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvPosOffsetReg_t;

/** @brief 中断触发波形点数寄存器 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvIntNumWaveReg_t;

/**
 * @brief 中断控制寄存器 01 位域定义
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t driver_num   : 3;      /**< 驱动器编号 */
        uint8_t int_en       : 1;      /**< 中断使能 */
        uint8_t addr0_hit    : 1;      /**< 地址 0 匹配标志 */
        uint8_t addr1_hit    : 1;      /**< 地址 1 匹配标志 */
        uint8_t reserved_6_7 : 2;      /**< 保留位 */
    } bits;
} WG_DrvIntReg01_t;

/** @brief 中断控制寄存器 02 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvIntReg02_t;

/** @brief 中断控制寄存器 03 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
} WG_DrvIntReg03_t;

/** @brief 交替模式周期限制寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvAltLimReg_t;

/** @brief 交替模式静默间隔寄存器（16-bit） */
typedef union {
    uint16_t value;                 /**< 整字访问 */
} WG_DrvAltSilentLimReg_t;

/**
 * @brief 驱动控制寄存器 2 位域定义
 */
typedef union {
    uint8_t value;                  /**< 整字节访问 */
    struct {
        uint8_t idac_msb    : 4;       /**< IDAC 高 4 位 */
        uint8_t out_pos     : 3;       /**< 输出位置（电流档位） */
        uint8_t reserved_7  : 1;       /**< 保留位 */
    } bits;
} WG_DriveRegCtrl2_t;

/* ============================================================================
 *  复合结构体（LOD/SCD/Waveform 配置）
 * ===========================================================================*/

/**
 * @brief 导联脱落检测（LOD）配置结构体
 *
 * 包含 LOD 所有相关寄存器的值，用于批量配置。
 */
typedef struct
{
    int CHANNEL;                        /**< 通道号 */
    LEAD_OFF_CTRL_t        lead_off_ctrl;           /**< LOD 控制寄存器（0x30） */
    LEAD_OFF_THR_H_0_t     lead_off_thr_h_0;        /**< 高阈值低 8 位（0x31） */
    LEAD_OFF_THR_H_1_t     lead_off_thr_h_1;        /**< 高阈值高 4 位（0x32） */
    LEAD_OFF_THR_L_0_t     lead_off_thr_l_0;        /**< 低阈值低 8 位（0x33） */
    LEAD_OFF_THR_L_1_t     lead_off_thr_l_1;        /**< 低阈值高 4 位（0x34） */
    LEAD_OFF_DLY_TGT_t     lead_off_dly_tgt[4];     /**< 延迟目标（0x35~0x38） */
    LEAD_OFF_TGT_t         lead_off_tgt;            /**< 目标值（0x39） */
    LEAD_OFF_INT_t         lead_off_int;            /**< 中断控制（0x3A） */
    LEAD_OFF_ANA_t         lead_off_ana;            /**< 模拟比较器状态（0x3B） */
    ANA_ENABLE_REG_1_t     ANA_ENABLE_REG_1;        /**< CH1 模拟使能（0x41） */
    ANA_ENABLE_REG_2_t     ANA_ENABLE_REG_2;        /**< CH2 模拟使能（0x42） */
    ANA_GEN_REG_2_t        ANA_GEN_REG_2;           /**< CH1 VDAC 低 8 位（0x45） */
    ANA_GEN_REG_3_t        ANA_GEN_REG_3;           /**< CH1 VDAC 高位+配置（0x46） */
    ANA_GEN_REG_4_t        ANA_GEN_REG_4;           /**< CH2 VDAC 低 8 位（0x47） */
    ANA_GEN_REG_5_t        ANA_GEN_REG_5;           /**< CH2 VDAC 高位+配置（0x48） */
} Leadoff_TypeDef;

/**
 * @brief 短路检测（SCD）配置结构体
 *
 * 包含 SCD 所有相关寄存器的值，用于批量配置。
 */
typedef struct
{
    int CHANNEL;                        /**< 通道号 */
    LEAD_OFF_CTRL_t         lead_off_ctrl;           /**< LOD 控制寄存器（0x30） */
    ANA_ENABLE_REG_0_t      ANA_ENABLE_REG_0;        /**< 模拟使能 0（0x40） */
    ANA_ENABLE_REG_1_t      ANA_ENABLE_REG_1;        /**< CH1 模拟使能（0x41） */
    ANA_ENABLE_REG_2_t      ANA_ENABLE_REG_2;        /**< CH2 模拟使能（0x42） */
    ANA_ENABLE_REG_3_t      ANA_ENABLE_REG_3;        /**< BIST 使能（0x43） */
    ANA_GEN_REG_1_t         ANA_GEN_REG_1;           /**< LVD 电压选择（0x44） */
    ANA_GEN_REG_2_t         ANA_GEN_REG_2;           /**< CH1 VDAC 低 8 位（0x45） */
    ANA_GEN_REG_3_t         ANA_GEN_REG_3;           /**< CH1 VDAC 高位+配置（0x46） */
    ANA_GEN_REG_4_t         ANA_GEN_REG_4;           /**< CH2 VDAC 低 8 位（0x47） */
    ANA_GEN_REG_5_t         ANA_GEN_REG_5;           /**< CH2 VDAC 高位+配置（0x48） */
    ANA_INTR_EN_t           ANA_INTR_EN;             /**< 中断使能（0x52） */
    ANA_INTR_STS_REG_t      ANA_INTR_STS_REG;        /**< 中断状态（0x53） */
    ANA_INT_COMP_POL_t      ANA_INT_COMP_POL;        /**< 比较器极性（0x54） */
    ANA_INT_STOP_WAVEGEN_t  ANA_INT_STOP_WAVEGEN;    /**< 停止波形发生器（0x55） */
    ANA_INT_SIM0_A00_t      ANA_INT_SIM0_A00;        /**< 刺激 0 地址 A00（0x56） */
    ANA_INT_SIM0_A01_t      ANA_INT_SIM0_A01;        /**< 刺激 0 地址 A01（0x57） */
    ANA_INT_SIM1_A10_t      ANA_INT_SIM1_A10;        /**< 刺激 1 地址 A10（0x58） */
    ANA_INT_SIM1_A11_t      ANA_INT_SIM1_A11;        /**< 刺激 1 地址 A11（0x59） */
    ANA_INT_CH1_INT_NUMBER_t ANA_INT_CH1_INT_NUMBER; /**< CH1 中断点数（0x5A） */
    ANA_INT_SIM2_A20_t      ANA_INT_SIM2_A20;        /**< 刺激 2 地址 A20（0x5B） */
    ANA_INT_SIM2_A21_t      ANA_INT_SIM2_A21;        /**< 刺激 2 地址 A21（0x5C） */
    ANA_INT_SIM3_A30_t      ANA_INT_SIM3_A30;        /**< 刺激 3 地址 A30（0x5D） */
    ANA_INT_SIM3_A31_t      ANA_INT_SIM3_A31;        /**< 刺激 3 地址 A31（0x5E） */
    ANA_INT_CH2_INT_NUMBER_t ANA_INT_CH2_INT_NUMBER; /**< CH2 中断点数（0x5F） */
    ANA_INTR_SIM_CL_t       ANA_INTR_SIM_CL;         /**< 刺激中断清除（0x60） */
} Short_detect_TypeDef;

/**
 * @brief 波形发生器配置结构体
 *
 * 包含波形发生器所有相关寄存器的值，用于 nnc6521_wavegen_config() 函数。
 */
typedef struct {
    int CHANNEL;                                        /**< 通道号（0 或 1） */
    WG_DrvConfigReg0_t              WG_DRV_CONFIG_REG0;           /**< 驱动配置（0x00） */
    WG_DrvCtrlReg0_t                WG_DRV_CTRL_REG0;             /**< 驱动控制（0x01） */
    WG_DrvPointConfig_t             WG_DRV_POINT_CONFIG;          /**< 采样点数（0x02） */
    WG_DrvInWaveAddrReg_t           WG_DRV_IN_WAVE_ADDR;          /**< 波形写入地址（0x03） */
    WG_DrvInWaveReg_t               WG_DRV_IN_WAVE;               /**< 波形写入数据（0x04） */
    WG_DrvRestClkReg_t              WG_DRV_REST_CLK;              /**< 休息时钟（0x05~0x06） */
    WG_DrvSilentClkReg_t            WG_DRV_SILENT_CLK;            /**< 静默时钟（0x07~0x09） */
    WG_DrvHalfWaveClkPntReg_t       WG_DRV_HALF_WAVE_CLK_PNT;     /**< 正半波周期（0x0A~0x0B） */
    WG_DrvNegHalfWaveClkPntReg_t    WG_DRV_NEG_HALF_WAVE_CLK_PNT; /**< 负半波周期（0x0C~0x0D） */
    WG_DrvDelayLimReg_t             WG_DRV_DELAY_LIM;             /**< 延迟限制（0x20~0x21） */
    WG_DrvNegScaleReg_t             WG_DRV_NEG_SCALE;             /**< 负半波缩放（0x22） */
    WG_DrvNegOffsetReg_t            WG_DRV_NEG_OFFSET;            /**< 负半波偏移（0x23） */
    WG_DrvPosScaleReg_t             WG_DRV_POS_SCALE;             /**< 正半波缩放（0x24） */
    WG_DrvPosOffsetReg_t            WG_DRV_POS_OFFSET;            /**< 正半波偏移（0x25） */
    WG_DrvIntNumWaveReg_t           WG_DRV_INT_NUM_WAVE;          /**< 中断波形点数（0x27） */
    WG_DrvIntReg01_t                WG_DRV_INT_REG01;             /**< 中断控制 01（0x28） */
    WG_DrvIntReg02_t                WG_DRV_INT_REG02;             /**< 中断控制 02（0x29） */
    WG_DrvIntReg03_t                WG_DRV_INT_REG03;             /**< 中断控制 03（0x2A） */
    WG_DrvAltLimReg_t               WG_DRV_ALT_LIM;               /**< 交替周期（0x2B~0x2C） */
    WG_DrvAltSilentLimReg_t         WG_DRV_ALT_SILENT_LIM;        /**< 交替静默间隔（0x2D~0x2E） */
    WG_DriveRegCtrl2_t              WG_DRIVE_REG_CTRL2;           /**< 驱动控制 2（0x31） */
} waveform_TypeDef;

#endif /* __NNC6521_REG_H__ */
