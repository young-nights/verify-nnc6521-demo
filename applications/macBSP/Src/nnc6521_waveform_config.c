/**
  ******************************************************************************
  * @file    nnc6521_waveform_config.c
  * @brief   NNC6521 waveform configuration for DJM-V10 beauty device.
  *          Implements 9 predefined waveforms with current mapping and
  *          NNC6521 driver API integration.
  *          All comments in English.
  ******************************************************************************
  */

#include "nnc6521.h"
#include "nnc6521_waveform_config.h"
#include <rtthread.h>

/* ============================================================================
 *  Custom waveform data arrays for specific waveforms
 * ===========================================================================*/

/**
 * @brief  Burst pulse waveform (64 points).
 *         First 32 points at full amplitude (burst ON), last 32 at zero (burst OFF).
 *         Used by Waveform 2 (Burst Train) to create 10 Hz burst envelope
 *         within 50 Hz carrier.
 */
static float burst_pulse_waveform_64[64] =
{
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

/**
 * @brief  Deep sculpt pulse waveform (128 points).
 *         Alternating high/low pattern simulating 4 kHz carrier modulated
 *         by 40~50 Hz envelope. Each pair of points forms one carrier cycle.
 *         Envelope: first 80 points active, last 48 points decay.
 */
static float deep_sculpt_pulse_128[128] =
{
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    /* Decay region: carrier amplitude tapers off */
    0.9f, 0.9f, 0.8f, 0.8f, 0.7f, 0.7f, 0.6f, 0.6f,
    0.5f, 0.5f, 0.4f, 0.4f, 0.3f, 0.3f, 0.2f, 0.2f,
    0.1f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

/* ============================================================================
 *  Global waveform configuration array
 *  PCLK = 2 MHz
 *  half_wave_clk = PCLK / (2 * frequency)
 *  silent_time = pulse_width_us * 2  (since PCLK = 2 MHz, 1 us = 2 clocks)
 * ===========================================================================*/

const waveform_config_t g_waveform_configs[WAVEFORM_COUNT] =
{
    /* ---- Waveform 1: Power Smooth ---- */
    {
        .id              = 1,
        .name            = "Power Smooth",
        .description     = "Strong smoothing symmetric square wave",
        .min_current     = 30,
        .max_current     = 80,
        .frequency       = 50,
        .pulse_width_us  = 300,
        .waveform_type   = WAVEFORM_TYPE_SQUARE,
        .gen_method      = GEN_METHOD_PRELOADED,
        .point_num       = 64,
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) */
        .silent_time     = 600,     /* 300 * 2 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL     /* Preloaded, no custom data */
    },

    /* ---- Waveform 2: Burst Train ---- */
    {
        .id              = 2,
        .name            = "Burst Train",
        .description     = "Burst pulse train at 10 Hz repetition",
        .min_current     = 30,
        .max_current     = 80,
        .frequency       = 50,
        .pulse_width_us  = 300,
        .waveform_type   = WAVEFORM_TYPE_BURST,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 64,
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) */
        .silent_time     = 600,     /* 300 * 2 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = burst_pulse_waveform_64
    },

    /* ---- Waveform 3: Gentle Smooth ---- */
    {
        .id              = 3,
        .name            = "Gentle Smooth",
        .description     = "Gentle smoothing symmetric square wave",
        .min_current     = 20,
        .max_current     = 60,
        .frequency       = 35,
        .pulse_width_us  = 300,
        .waveform_type   = WAVEFORM_TYPE_SQUARE,
        .gen_method      = GEN_METHOD_PRELOADED,
        .point_num       = 64,
        .half_wave_clk   = 28571,   /* 2000000 / (2*35) */
        .silent_time     = 600,     /* 300 * 2 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL
    },

    /* ---- Waveform 4: Deep Sculpt ---- */
    {
        .id              = 4,
        .name            = "Deep Sculpt",
        .description     = "Deep sculpting with 4 kHz carrier",
        .min_current     = 30,
        .max_current     = 80,
        .frequency       = 50,
        .pulse_width_us  = 250,     /* 4 kHz carrier period */
        .waveform_type   = WAVEFORM_TYPE_BALANCED_SQUARE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 128,
        .half_wave_clk   = 20000,   /* 2000000 / (2*50) */
        .silent_time     = 250,     /* 2000000 / (2*4000) */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = deep_sculpt_pulse_128
    },

    /* ---- Waveform 5: Soft Sculpt ---- */
    {
        .id              = 5,
        .name            = "Soft Sculpt",
        .description     = "Soft sculpting sine wave",
        .min_current     = 30,
        .max_current     = 80,
        .frequency       = 40,
        .pulse_width_us  = 0,
        .waveform_type   = WAVEFORM_TYPE_SINE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 128,
        .half_wave_clk   = 25000,   /* 2000000 / (2*40) */
        .silent_time     = 0,
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = normalized_sine_waveform_128
    },

    /* ---- Waveform 6: Circulation Sculpt ---- */
    {
        .id              = 6,
        .name            = "Circulation Sculpt",
        .description     = "Circulation sculpting with 4 kHz carrier AM",
        .min_current     = 20,
        .max_current     = 60,
        .frequency       = 10,
        .pulse_width_us  = 0,
        .waveform_type   = WAVEFORM_TYPE_BALANCED_SINE,
        .gen_method      = GEN_METHOD_AMPLITUDE_MOD,
        .point_num       = 64,
        .half_wave_clk   = 0,       /* AM mode uses carrier_clk */
        .silent_time     = 0,
        .rest_time       = 0,
        .carrier_clk     = 250,     /* 2000000 / (2*4000) */
        .am_interval     = 3125,    /* 2000000 / (64*10) */
        .waveform_data   = normalized_sine_waveform_64
    },

    /* ---- Waveform 7: Smooth & Firm ---- */
    {
        .id              = 7,
        .name            = "Smooth & Firm",
        .description     = "Smooth and firm triangle wave",
        .min_current     = 15,
        .max_current     = 50,
        .frequency       = 100,
        .pulse_width_us  = 400,
        .waveform_type   = WAVEFORM_TYPE_TRIANGLE,
        .gen_method      = GEN_METHOD_PRELOADED,
        .point_num       = 64,
        .half_wave_clk   = 10000,   /* 2000000 / (2*100) */
        .silent_time     = 800,     /* 400 * 2 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL
    },

    /* ---- Waveform 8: Lymphatic Drainage ---- */
    {
        .id              = 8,
        .name            = "Lymphatic Drainage",
        .description     = "Low-frequency sine for lymphatic drainage",
        .min_current     = 15,
        .max_current     = 40,
        .frequency       = 5,
        .pulse_width_us  = 450,
        .waveform_type   = WAVEFORM_TYPE_SINE,
        .gen_method      = GEN_METHOD_CUSTOM_SPI,
        .point_num       = 64,
        .half_wave_clk   = 200000,  /* 2000000 / (2*5) */
        .silent_time     = 900,     /* 450 * 2 */
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = normalized_sine_waveform_64
    },

    /* ---- Waveform 9: Soothing Ending ---- */
    {
        .id              = 9,
        .name            = "Soothing Ending",
        .description     = "Soothing ending sine wave",
        .min_current     = 10,
        .max_current     = 30,
        .frequency       = 10,
        .pulse_width_us  = 0,
        .waveform_type   = WAVEFORM_TYPE_SINE,
        .gen_method      = GEN_METHOD_PRELOADED,
        .point_num       = 64,
        .half_wave_clk   = 100000,  /* 2000000 / (2*10) */
        .silent_time     = 0,
        .rest_time       = 0,
        .carrier_clk     = 0,
        .am_interval     = 0,
        .waveform_data   = NULL
    }
};

/* ============================================================================
 *  Helper: Map preloaded waveform type to NNC6521 WaveformSelect_t
 * ===========================================================================*/
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
 *  Public: Calculate actual current from percentage
 * ===========================================================================*/
uint32_t waveform_calc_current(uint8_t waveform_id, uint8_t percent)
{
    if (waveform_id < 1 || waveform_id > WAVEFORM_COUNT) return 0;
    if (percent > 100) percent = 100;

    const waveform_config_t *cfg = &g_waveform_configs[waveform_id - 1];
    uint32_t range = cfg->max_current - cfg->min_current;
    return cfg->min_current + (range * percent) / 100;
}

/* ============================================================================
 *  Public: Get waveform configuration by ID
 * ===========================================================================*/
const waveform_config_t* waveform_get_config(uint8_t waveform_id)
{
    if (waveform_id < 1 || waveform_id > WAVEFORM_COUNT) return NULL;
    return &g_waveform_configs[waveform_id - 1];
}

/* ============================================================================
 *  Public: Apply waveform to NNC6521
 * ===========================================================================*/
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
                nnc6521_customized_waveform(chip_id, channel,
                                            cfg->point_num,
                                            cfg->waveform_data,
                                            actual_current,
                                            (uint16_t)cfg->half_wave_clk,
                                            (uint16_t)cfg->half_wave_clk,
                                            cfg->silent_time,
                                            cfg->rest_time,
                                            0);  /* asymmetric */
            }
            break;

        case GEN_METHOD_AMPLITUDE_MOD:
            if (cfg->waveform_data != NULL) {
                nnc6521_amplitude_modulation(chip_id, channel,
                                             cfg->point_num,
                                             cfg->waveform_data,
                                             actual_current,
                                             cfg->carrier_clk,
                                             cfg->silent_time,
                                             cfg->am_interval);
            }
            break;

        default:
            break;
    }
}
