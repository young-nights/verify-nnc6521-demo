/**
  ******************************************************************************
  * @file    nnc6521_waveform_config.c
  * @brief   NNC6521 波形配置文件，用于 DJM-V10 美容设备。
  *          实现 9 种预定义波形的电流映射和 NNC6521 驱动 API 集成。
  *          包含波形参数计算、配置查询和波形应用功能。
  ******************************************************************************
  */

#include "nnc6521.h"
#include "nnc6521_waveform_config.h"
#include <rtthread.h>

/* ============================================================================
 *  自定义波形数据数组
 * ===========================================================================*/

/**
 * @brief 突发脉冲波形（64 点）
 *
 * 波形结构：
 * - 前 32 个点：幅值 1.0（突发 ON，满幅输出）
 * - 后 32 个点：幅值 0.0（突发 OFF，无输出）
 *
 * 用于 Waveform 2（Burst Train），在 50 Hz 载波内实现 10 Hz 突发包络。
 * 占空比：50%（32/64），产生间歇性刺激效果。
 *
 * @see g_waveform_configs[1]（Waveform 2 配置）
 */
static float burst_pulse_waveform_64[64] =
{
    /* 50Hz carrier, 300us pulse width (~1 point per cycle).
     * Alternating: 1 point pulse + 31 points zero per half-wave.
     * Each point = 20000/32 = 625 PCLK = 312.5us.
     * Pulse at point 0 of each half-wave, rest zero. */
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

/**
 * @brief 深层塑形脉冲波形（128 点）
 *
 * 波形结构：
 * - 前 80 个点：幅值 1.0（载波活跃区域）
 * - 后 48 个点：从 0.9 线性衰减到 0.0（衰减区域）
 *
 * 衰减规律：每 2 个点为一组，幅度递减 0.1（0.9→0.8→0.7→...→0.0）。
 * 模拟 4 kHz 载波被 40~50 Hz 包络调制的效果。
 * 每对相邻点构成一个载波周期。
 *
 * 用于 Waveform 4（Deep Sculpt），实现深层组织塑形刺激。
 *
 * @see g_waveform_configs[3]（Waveform 4 配置）
 */
static float deep_sculpt_pulse_128[128] =
{
    0.000000f, 0.024541f, 0.000000f, 0.073565f, 0.098017f, 0.000000f, 0.146730f, 0.170962f,
    0.000000f, 0.219101f, 0.242980f, 0.000000f, 0.290285f, 0.313682f, 0.000000f, 0.359895f,
    0.382683f, 0.000000f, 0.427555f, 0.449611f, 0.000000f, 0.492898f, 0.514103f, 0.000000f,
    0.555570f, 0.575808f, 0.000000f, 0.615232f, 0.634393f, 0.000000f, 0.671559f, 0.689541f,
    0.000000f, 0.724247f, 0.740951f, 0.000000f, 0.773010f, 0.788346f, 0.000000f, 0.817585f,
    0.831470f, 0.000000f, 0.857729f, 0.870087f, 0.000000f, 0.893224f, 0.903989f, 0.000000f,
    0.923880f, 0.932993f, 0.000000f, 0.949528f, 0.956940f, 0.000000f, 0.970031f, 0.975702f,
    0.000000f, 0.985278f, 0.989177f, 0.000000f, 0.995185f, 0.997290f, 0.000000f, 0.999699f,
    1.000000f, 0.000000f, 0.998795f, 0.997290f, 0.000000f, 0.992480f, 0.989177f, 0.000000f,
    0.000000f, 0.914210f, 0.903989f, 0.000000f, 0.881921f, 0.870087f, 0.000000f, 0.844854f,
    0.831470f, 0.000000f, 0.803208f, 0.788346f, 0.000000f, 0.757209f, 0.740951f, 0.000000f,
    0.707107f, 0.689541f, 0.000000f, 0.653173f, 0.634393f, 0.000000f, 0.595699f, 0.575808f,
    0.000000f, 0.534998f, 0.514103f, 0.000000f, 0.471397f, 0.449611f, 0.000000f, 0.405241f,
    0.382683f, 0.000000f, 0.336890f, 0.313682f, 0.000000f, 0.266713f, 0.242980f, 0.000000f,
    0.195090f, 0.170962f, 0.000000f, 0.122411f, 0.098017f, 0.000000f, 0.049068f, 0.024541f
};

/* ============================================================================
 *  全局波形配置数组
 *
 *  PCLK = 2 MHz
 *  half_wave_clk = PCLK / (2 * frequency)
 *  silent_time = pulse_width_us * 2  （因 PCLK = 2 MHz，1 us = 2 个时钟）
 * ===========================================================================*/

const waveform_config_t g_waveform_configs[WAVEFORM_COUNT] =
{
    /* ---- Waveform 1: Power Smooth（强力平滑）---- */
    {
        .id              = 1,
        .name            = "Power Smooth",
        .description     = "Strong smoothing symmetric square wave",
        .min_current     = 3,        /* 最小输出电流 3 mA */
        .max_current     = 8,        /* 最大输出电流 8 mA */
        .frequency       = 50,       /* 波形频率 50 Hz */
        .pulse_width_us  = 300,      /* 脉冲宽度 300 us */
        .waveform_type   = WAVEFORM_TYPE_SQUARE,     /* 方波类型 */
        .gen_method      = GEN_METHOD_PRELOADED,      /* 使用预加载波形 */
        .point_num       = 64,       /* 64 点采样 */
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) = 20000 */
        .silent_time     = 600,     /* 300 * 2 = 600 */
        .rest_time       = 0,       /* 无死区 */
        .carrier_clk     = 0,       /* 非 AM 模式 */
        .am_interval     = 0,       /* 非 AM 模式 */
        .waveform_data   = NULL     /* 预加载模式，无需自定义数据 */
    },

    /* ---- Waveform 2: Burst Train（突发脉冲串）---- */
    {
        .id              = 2,
        .name            = "Burst Train",
        .description     = "Burst pulse train at 10 Hz repetition",
        .min_current     = 3,
        .max_current     = 8,
        .frequency       = 50,       /* 载波频率 50 Hz */
        .pulse_width_us  = 300,
        .waveform_type   = WAVEFORM_TYPE_BURST,       /* 突发类型 */
        .gen_method      = GEN_METHOD_CUSTOM_SPI,      /* 使用 SPI 自定义波形 */
        .point_num       = 64,
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) = 20000 */
        .silent_time     = 600,     /* 300 * 2 = 600 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = burst_pulse_waveform_64  /* 突发脉冲数据 */
    },

    /* ---- Waveform 3: Gentle Smooth（柔和光滑）---- */
    {
        .id              = 3,
        .name            = "Gentle Smooth",
        .description     = "Gentle smoothing symmetric square wave",
        .min_current     = 2,        /* 较低的最小电流 */
        .max_current     = 6,        /* 较低的最大电流 */
        .frequency       = 35,       /* 较低频率 35 Hz */
        .pulse_width_us  = 300,
        .waveform_type   = WAVEFORM_TYPE_SQUARE,
        .gen_method      = GEN_METHOD_PRELOADED,
        .point_num       = 64,
        .half_wave_clk   = 28571,   /* 2000000 / (2*35) = 28571 */
        .silent_time     = 600,     /* 300 * 2 = 600 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL
    },

    /* ---- Waveform 4: Deep Sculpt（深层塑形）---- */
    {
        .id              = 4,
        .name            = "Deep Sculpt",
        .description     = "Deep sculpting with 4 kHz carrier",
        .min_current     = 3,
        .max_current     = 8,
        .frequency       = 50,       /* 包络频率 50 Hz */
        .pulse_width_us  = 250,      /* 4 kHz 载波半周期 250 us */
        .waveform_type   = WAVEFORM_TYPE_BALANCED_SQUARE, /* 平衡方波 */
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 128,      /* 128 点高精度 */
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) = 20000 */
        .silent_time     = 250,     /* 2000000 / (2*4000) = 250（载波周期） */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = deep_sculpt_pulse_128  /* 深层塑形脉冲数据 */
    },

    /* ---- Waveform 5: Soft Sculpt（柔和塑形）---- */
    {
        .id              = 5,
        .name            = "Soft Sculpt",
        .description     = "Soft sculpting sine wave",
        .min_current     = 3,
        .max_current     = 8,
        .frequency       = 40,       /* 正弦波频率 40 Hz */
        .pulse_width_us  = 0,        /* 正弦波无脉冲宽度 */
        .waveform_type   = WAVEFORM_TYPE_SINE,          /* 正弦波 */
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 128,      /* 128 点高精度正弦 */
        .half_wave_clk   = 25000,   /* 2000000 / (2*40) = 25000 */
        .silent_time     = 0,       /* 无静默期 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = normalized_sine_waveform_128  /* 128 点正弦数据 */
    },

    /* ---- Waveform 6: Circulation Sculpt（循环塑形）---- */
    /* Changed from AMPLITUDE_MOD to CUSTOM_SPI: pre-computed AM waveform bypasses hardware AM mode */
    {
        .id              = 6,
        .name            = "Circulation Sculpt",
        .description     = "Circulation sculpting with 4kHz carrier AM",
        .min_current     = 2,
        .max_current     = 6,
        .frequency       = 10,       /* 包络频率 10 Hz */
        .pulse_width_us  = 0,
        .waveform_type   = WAVEFORM_TYPE_BALANCED_SINE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,       /* Changed from AMPLITUDE_MOD */
        .point_num       = 64,
        .half_wave_clk   = 12500,   /* PCLK/8: 250000 / (2*10) = 12500 */
        .silent_time     = 0,
        .rest_time       = 0,
        .carrier_clk     = 0,        /* Not used in SPI mode */
        .am_interval     = 0,        /* Not used in SPI mode */
        .waveform_data   = circulation_sculpt_am_64  /* Pre-computed AM data */
    },

    /* ---- Waveform 7: Smooth & Firm（光滑紧致）---- */
    {
        .id              = 7,
        .name            = "Smooth & Firm",
        .description     = "Smooth and firm triangle wave",
        .min_current     = 1,        /* 较低电流 */
        .max_current     = 5,
        .frequency       = 100,      /* 高频 100 Hz */
        .pulse_width_us  = 400,      /* 脉冲宽度 400 us */
        .waveform_type   = WAVEFORM_TYPE_TRIANGLE,       /* 三角波 */
        .gen_method      = GEN_METHOD_PRELOADED,          /* 预加载模式 */
        .point_num       = 64,
        .half_wave_clk   = 10000,   /* 2000000 / (2*100) = 10000 */
        .silent_time     = 800,     /* 400 * 2 = 800 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL
    },

    /* ---- Waveform 8: Lymphatic Drainage（淋巴引流）---- */
    /* NOTE: Requires PCLK_DIV_16 (PCLK=125kHz). With PCLK/16:
     *   half_wave_clk = 125000 / (2*5) = 12500 (fits uint16_t)
     *   silent_time   = 125000 * 450e-6 ≈ 56 */
    {
        .id              = 8,
        .name            = "Lymphatic Drainage",
        .description     = "Low-frequency sine for lymphatic drainage",
        .min_current     = 1,        /* 低电流 */
        .max_current     = 4,
        .frequency       = 5,        /* 极低频率 5 Hz */
        .pulse_width_us  = 450,
        .waveform_type   = WAVEFORM_TYPE_SINE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 64,
        .half_wave_clk   = 12500,   /* PCLK/16: 125000/(2*5) = 12500 */
        .silent_time     = 56,      /* PCLK/16: 125000*450e-6 ≈ 56 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = normalized_sine_waveform_64   /* 64 点正弦 */
    },

    /* ---- Waveform 9: Soothing Ending（舒缓收尾）---- */
    /* NOTE: Requires PCLK_DIV_8 (PCLK=250kHz). With PCLK/8:
     *   half_wave_clk = 250000 / (2*10) = 12500 (fits uint16_t)
     *   Uses custom SPI sine (not preloaded) per spec */
    {
        .id              = 9,
        .name            = "Soothing Ending",
        .description     = "Soothing ending sine wave",
        .min_current     = 1,        /* 最低电流 */
        .max_current     = 3,
        .frequency       = 10,       /* 低频 10 Hz */
        .pulse_width_us  = 0,        /* 无脉冲宽度 */
        .waveform_type   = WAVEFORM_TYPE_SINE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,         /* Custom SPI sine */
        .point_num       = 64,
        .half_wave_clk   = 12500,   /* PCLK/8: 250000/(2*10) = 12500 */
        .silent_time     = 0,       /* 连续正弦波无静默期 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = normalized_sine_waveform_64    /* 64 点正弦 */
    }
};

/* ============================================================================
 *  内部辅助函数：PCLK 分频器设置
 * ===========================================================================*/

/**
 * @brief Set PCLK divider for a NNC6521 chip.
 *        Used for low-frequency waveforms that exceed 16-bit half-wave register.
 */
static void set_pclk_divider(uint8_t chip_id, uint8_t divider)
{
    nnc6521_write_reg(chip_id, CLK_CTRL_REG_ADDR, divider);
}

/* ============================================================================
 *  内部辅助函数：波形类型到 NNC6521 预加载枚举的映射
 * ===========================================================================*/

/**
 * @brief 将波形类型映射为 NNC6521 预加载波形枚举值
 *
 * @param[in] waveform_type 波形类型（WAVEFORM_TYPE_xxx）
 * @return NNC6521 预加载波形枚举（WAVEFORM_PULSE / WAVEFORM_TRIANGLE / WAVEFORM_SINE）
 *
 * @note 方波和突发类型映射为 WAVEFORM_PULSE
 * @note 正弦和平衡正弦类型映射为 WAVEFORM_SINE
 */
static uint8_t get_preloaded_type(uint8_t waveform_type)
{
    switch (waveform_type) {
        case WAVEFORM_TYPE_SQUARE:
        case WAVEFORM_TYPE_BURST:
            return WAVEFORM_PULSE;
        case WAVEFORM_TYPE_TRIANGLE:
            return WAVEFORM_TRIANGLE;
        case WAVEFORM_TYPE_SINE:
        case WAVEFORM_TYPE_BALANCED_SINE:
            return WAVEFORM_SINE;
        default:
            return WAVEFORM_SINE;
    }
}

/* ============================================================================
 *  公共接口：计算实际输出电流
 * ===========================================================================*/

/**
 * @brief 根据波形 ID 和百分比计算实际输出电流
 *
 * 计算公式：actual_current = min_current + (max_current - min_current) * percent / 100
 *
 * @param[in] waveform_id 波形编号（1~WAVEFORM_COUNT）
 * @param[in] percent     电流百分比（0~100）
 * @return 实际输出电流（单位：mA），参数无效时返回 0
 *
 * @see waveform_apply()
 */
uint32_t waveform_calc_current(uint8_t waveform_id, uint8_t percent)
{
    if (waveform_id < 1 || waveform_id > WAVEFORM_COUNT) return 0;
    if (percent > 100) percent = 100;

    const waveform_config_t *cfg = &g_waveform_configs[waveform_id - 1];
    uint32_t range = cfg->max_current - cfg->min_current;
    return cfg->min_current + (range * percent) / 100;
}

/* ============================================================================
 *  公共接口：获取波形配置
 * ===========================================================================*/

/**
 * @brief 根据波形 ID 获取配置结构体指针
 *
 * @param[in] waveform_id 波形编号（1~WAVEFORM_COUNT）
 * @return 配置结构体指针，参数无效时返回 NULL
 *
 * @see g_waveform_configs
 */
const waveform_config_t* waveform_get_config(uint8_t waveform_id)
{
    if (waveform_id < 1 || waveform_id > WAVEFORM_COUNT) return NULL;
    return &g_waveform_configs[waveform_id - 1];
}

/* ============================================================================
 *  公共接口：应用波形到 NNC6521
 * ===========================================================================*/

/**
 * @brief 将指定波形应用到 NNC6521 芯片
 *
 * 根据波形配置的生成方法（gen_method）自动选择对应的 NNC6521 API：
 * - GEN_METHOD_PRELOADED：调用 nnc6521_preloaded_waveform()，使用内置波形
 * - GEN_METHOD_CUSTOM_SPI：调用 nnc6521_customized_waveform()，传输自定义数据
 * - GEN_METHOD_AMPLITUDE_MOD：调用 nnc6521_amplitude_modulation()，AM 调制
 *
 * @param[in] chip_id       芯片编号
 * @param[in] channel       通道编号
 * @param[in] waveform_id   波形编号（1~WAVEFORM_COUNT）
 * @param[in] percent       电流百分比（0~100）
 *
 * @note CI（电流索引）默认值为 4，提供合理的驱动范围
 * @note 自定义 SPI 波形使用非对称模式（asymmetric = 0）
 *
 * @see nnc6521_preloaded_waveform()
 * @see nnc6521_customized_waveform()
 * @see nnc6521_amplitude_modulation()
 * @see waveform_calc_current()
 */
void waveform_apply(uint8_t chip_id, uint8_t channel,
                    uint8_t waveform_id, uint8_t percent)
{
    if (waveform_id < 1 || waveform_id > WAVEFORM_COUNT) return;

    const waveform_config_t *cfg = &g_waveform_configs[waveform_id - 1];
    uint32_t actual_current = waveform_calc_current(waveform_id, percent);

    /* Map waveform type to NNC6521 preloaded waveform enum */
    uint8_t nnc_waveform = get_preloaded_type(cfg->waveform_type);

    /* CI (current index) for drive strength control.
     * Default value 4 provides reasonable drive range. */
    uint8_t ci = 4;

    switch (cfg->gen_method) {
        case GEN_METHOD_PRELOADED:
            nnc6521_preloaded_waveform(chip_id, channel,
                                       nnc_waveform,
                                       cfg->point_num,
                                       ci,
                                       (uint16_t)cfg->half_wave_clk,
                                       (uint16_t)cfg->half_wave_clk,
                                       cfg->silent_time,
                                       cfg->rest_time);
            break;

        case GEN_METHOD_CUSTOM_SPI:
            if (cfg->waveform_data != NULL) {
                /* Low-freq waveforms need reduced PCLK to fit 16-bit register */
                if (waveform_id == 8) {
                    set_pclk_divider(chip_id, PCLK_DIV_16);
                } else if (waveform_id == 6 || waveform_id == 9) {
                    set_pclk_divider(chip_id, PCLK_DIV_8);
                }

                nnc6521_customized_waveform(chip_id, channel,
                                            cfg->point_num,
                                            cfg->waveform_data,
                                            actual_current * 1000,
                                            (uint16_t)cfg->half_wave_clk,
                                            (uint16_t)cfg->half_wave_clk,
                                            cfg->silent_time,
                                            cfg->rest_time,
                                            0);  /* asymmetric */

                /* Restore PCLK to default */
                if (waveform_id == 6 || waveform_id == 8 || waveform_id == 9) {
                    set_pclk_divider(chip_id, PCLK_DIV_1);
                }
            }
            break;

        case GEN_METHOD_AMPLITUDE_MOD:
            if (cfg->waveform_data != NULL) {
                nnc6521_amplitude_modulation(chip_id, channel,
                                             cfg->point_num,
                                             cfg->waveform_data,
                                             actual_current * 1000,
                                             cfg->carrier_clk,
                                             cfg->silent_time,
                                             cfg->am_interval);
            }
            break;

        default:
            break;
    }
}
