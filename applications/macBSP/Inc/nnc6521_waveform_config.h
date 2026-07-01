/**
  ******************************************************************************
  * @file    nnc6521_waveform_config.h
  * @brief   NNC6521 waveform configuration for DJM-V10 beauty device.
  *          Defines 9 predefined waveforms with current mapping and
  *          NNC6521 driver API integration.
  *          All comments in English.
  ******************************************************************************
  */

#ifndef __NNC6521_WAVEFORM_CONFIG_H__
#define __NNC6521_WAVEFORM_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Number of predefined waveforms -------------------------------------------*/
#define WAVEFORM_COUNT          9

/* Default current percentage -----------------------------------------------*/
#define WAVEFORM_DEFAULT_PCT    50

/* Waveform generation method -----------------------------------------------*/
typedef enum {
    GEN_METHOD_PRELOADED = 0,   /* NNC6521 built-in sine/pulse/triangle */
    GEN_METHOD_CUSTOM_SPI = 1,  /* Custom SPI waveform array */
    GEN_METHOD_AMPLITUDE_MOD = 2 /* Amplitude modulation (envelope + carrier) */
} waveform_gen_method_t;

/* Waveform type identifiers ------------------------------------------------*/
typedef enum {
    WAVEFORM_TYPE_SQUARE = 0,       /* Symmetric square wave */
    WAVEFORM_TYPE_BURST = 1,        /* Burst pulse train */
    WAVEFORM_TYPE_TRIANGLE = 2,     /* Triangle wave */
    WAVEFORM_TYPE_SINE = 3,         /* Sine wave */
    WAVEFORM_TYPE_BALANCED_SQUARE = 4, /* Balanced square (carrier) */
    WAVEFORM_TYPE_BALANCED_SINE = 5    /* Balanced sine (carrier) */
} waveform_type_t;

/* Waveform configuration structure -----------------------------------------*/
typedef struct {
    uint8_t     id;              /* Waveform ID (1~9) */
    const char *name;            /* Waveform name string */
    const char *description;     /* Short description */
    uint32_t    min_current;     /* Minimum current in mA */
    uint32_t    max_current;     /* Maximum current in mA */
    uint16_t    frequency;       /* Main frequency in Hz */
    uint16_t    pulse_width_us;  /* Pulse width in microseconds */
    uint8_t     waveform_type;   /* Waveform type (waveform_type_t) */
    uint8_t     gen_method;      /* Generation method (waveform_gen_method_t) */
    uint8_t     point_num;       /* Sample point count (64 or 128) */
    uint16_t    half_wave_clk;   /* Half-wave clock cycles */
    uint32_t    silent_time;     /* Silent time in clock cycles */
    uint16_t    rest_time;       /* Rest time in clock cycles */
    uint16_t    carrier_clk;     /* Carrier half-wave clock (AM mode) */
    uint16_t    am_interval;     /* Envelope interval clock (AM mode) */
    float      *waveform_data;   /* Pointer to normalized waveform array */
} waveform_config_t;

/* Global waveform configuration array --------------------------------------*/
extern const waveform_config_t g_waveform_configs[WAVEFORM_COUNT];

/* ============================================================================
 *  Public API
 * ===========================================================================*/

/**
 * @brief  Apply waveform to NNC6521 by waveform ID and current percentage.
 * @param  chip_id    NNC6521_CHIP_1 or NNC6521_CHIP_2
 * @param  channel    WAVEFORM_GEN_CH0 or WAVEFORM_GEN_CH1
 * @param  waveform_id  Waveform ID (1~9)
 * @param  percent    Current percentage (0~100)
 */
void waveform_apply(uint8_t chip_id, uint8_t channel,
                    uint8_t waveform_id, uint8_t percent);

/**
 * @brief  Calculate actual current from waveform ID and percentage.
 * @param  waveform_id  Waveform ID (1~9)
 * @param  percent      Current percentage (0~100)
 * @return Actual current in mA
 */
uint32_t waveform_calc_current(uint8_t waveform_id, uint8_t percent);

/**
 * @brief  Get waveform configuration by ID.
 * @param  waveform_id  Waveform ID (1~9)
 * @return Pointer to waveform config, or NULL if invalid
 */
const waveform_config_t* waveform_get_config(uint8_t waveform_id);

#ifdef __cplusplus
}
#endif

#endif /* __NNC6521_WAVEFORM_CONFIG_H__ */
