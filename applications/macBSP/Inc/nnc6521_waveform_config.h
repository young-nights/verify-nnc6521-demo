/**
  ******************************************************************************
  * @file    nnc6521_waveform_config.h
  * @brief   NNC6521 波形配置模块头文件（DJM-V10 美容仪）
  *          定义 9 种预设波形，包含电流映射和 NNC6521 驱动 API 集成。
  ******************************************************************************
  */

#ifndef __NNC6521_WAVEFORM_CONFIG_H__
#define __NNC6521_WAVEFORM_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 预设波形数量 --------------------------------------------------------------*/
#define WAVEFORM_COUNT          9       /**< 预设波形总数（ID 1~9） */

/* 默认电流百分比 ------------------------------------------------------------*/
#define WAVEFORM_DEFAULT_PCT    50      /**< 默认电流百分比（0~100） */

/**
 * @brief 波形生成方式枚举
 */
typedef enum {
    GEN_METHOD_PRELOADED = 0,       /**< NNC6521 内置预加载波形（正弦/脉冲/三角波） */
    GEN_METHOD_CUSTOM_SPI = 1,      /**< 自定义 SPI 波形数组 */
    GEN_METHOD_AMPLITUDE_MOD = 2    /**< 幅度调制（包络 + 载波） */
} waveform_gen_method_t;

/**
 * @brief 波形类型标识枚举
 */
typedef enum {
    WAVEFORM_TYPE_SQUARE = 0,           /**< 对称方波 */
    WAVEFORM_TYPE_BURST = 1,            /**< 突发脉冲序列 */
    WAVEFORM_TYPE_TRIANGLE = 2,         /**< 三角波 */
    WAVEFORM_TYPE_SINE = 3,             /**< 正弦波 */
    WAVEFORM_TYPE_BALANCED_SQUARE = 4,  /**< 平衡方波（载波模式） */
    WAVEFORM_TYPE_BALANCED_SINE = 5     /**< 平衡正弦波（载波模式） */
} waveform_type_t;

/**
 * @brief 波形配置结构体
 *
 * 描述一种预设波形的全部参数，包括电流范围、频率、脉宽、生成方式等。
 * 通过 waveform_apply() 函数可直接应用到 NNC6521 芯片。
 */
typedef struct {
    uint8_t     id;              /**< 波形 ID（1~9） */
    const char *name;            /**< 波形名称字符串 */
    const char *description;     /**< 波形简要描述 */
    uint32_t    min_current;     /**< 最小输出电流（mA） */
    uint32_t    max_current;     /**< 最大输出电流（mA） */
    uint16_t    frequency;       /**< 主频率（Hz） */
    uint16_t    pulse_width_us;  /**< 脉冲宽度（微秒） */
    uint8_t     waveform_type;   /**< 波形类型（waveform_type_t） */
    uint8_t     gen_method;      /**< 生成方式（waveform_gen_method_t） */
    uint8_t     point_num;       /**< 每周期采样点数（64 或 128） */
    uint16_t    half_wave_clk;   /**< 半波时钟周期数（PCLK / (2 * freq)） */
    uint32_t    silent_time;     /**< 静默时间（时钟周期数） */
    uint16_t    rest_time;       /**< 休息时间（时钟周期数） */
    uint16_t    carrier_clk;     /**< 载波半波时钟（AM 模式使用） */
    uint16_t    am_interval;     /**< 包络间隔时钟（AM 模式使用） */
    float      *waveform_data;   /**< 归一化波形数组指针（预加载模式为 NULL） */
} waveform_config_t;

/* 全局波形配置数组 ----------------------------------------------------------*/
extern const waveform_config_t g_waveform_configs[WAVEFORM_COUNT];

/* ============================================================================
 *  公共 API
 * ===========================================================================*/

/**
 * @brief 按波形 ID 和电流百分比将波形应用到 NNC6521
 *
 * 根据波形 ID 查找配置，计算实际电流，然后调用对应的底层驱动函数
 * 输出波形。支持三种生成方式：预加载、自定义 SPI、幅度调制。
 *
 * @param[in] chip_id      芯片编号（NNC6521_CHIP_1 或 NNC6521_CHIP_2）
 * @param[in] channel      波形通道（WAVEFORM_GEN_CH0 或 WAVEFORM_GEN_CH1）
 * @param[in] waveform_id  波形 ID（1~9）
 * @param[in] percent      电流百分比（0~100）
 *
 * @note waveform_id 超出范围时函数直接返回，不做任何操作
 * @note percent 会被限制在 0~100 范围内
 *
 * @code
 * // 应用波形 1（Power Smooth），50% 电流
 * waveform_apply(NNC6521_CHIP_1, WAVEFORM_GEN_CH0, 1, 50);
 * @endcode
 *
 * @see waveform_calc_current, waveform_get_config
 */
void waveform_apply(uint8_t chip_id, uint8_t channel,
                    uint8_t waveform_id, uint8_t percent);

/**
 * @brief 根据波形 ID 和百分比计算实际输出电流
 *
 * 计算公式：actual_current = min_current + (max_current - min_current) * percent / 100
 *
 * @param[in] waveform_id  波形 ID（1~9）
 * @param[in] percent      电流百分比（0~100）
 *
 * @return 实际电流值（mA），无效 ID 返回 0
 *
 * @code
 * uint32_t ma = waveform_calc_current(1, 50);  // 返回 55 mA（(30+80)/2））
 * @endcode
 *
 * @see waveform_apply
 */
uint32_t waveform_calc_current(uint8_t waveform_id, uint8_t percent);

/**
 * @brief 根据波形 ID 获取配置结构体指针
 *
 * @param[in] waveform_id  波形 ID（1~9）
 *
 * @return 指向波形配置的常量指针，无效 ID 返回 NULL
 *
 * @code
 * const waveform_config_t *cfg = waveform_get_config(1);
 * if (cfg != NULL) {
 *     rt_kprintf("波形: %s, 频率: %d Hz\n", cfg->name, cfg->frequency);
 * }
 * @endcode
 *
 * @see waveform_apply
 */
const waveform_config_t* waveform_get_config(uint8_t waveform_id);

#ifdef __cplusplus
}
#endif

#endif /* __NNC6521_WAVEFORM_CONFIG_H__ */
