# NNC6521 Waveform Design Specification

| Field | Content |
|-------|---------|
| Document ID | NNC6521-WAVEFORM-SPEC-001 |
| Version | V1.0 |
| Date | 2026-07-01 |
| Author | Engineering Team |
| Status | Initial Release |

---

## 1. Project Overview

The verify-nnc6521-demo project is an NNC6521 dual-chip verification demo based on **STM32F103RCT6 + RT-Thread**. This project validates the functionality and performance of the NNC6521 stimulation waveform generator chip.

This document defines the detailed design for **9 predefined waveforms** based on the DJM-V10 microcurrent beauty device requirements, covering waveform specifications, current mapping, register configuration, and test verification.

---

## 2. Hardware Platform

### 2.1 MCU
- **Model**: STM32F103RCT6 (ARM Cortex-M3, 72 MHz)
- **RTOS**: RT-Thread

### 2.2 NNC6521 Dual-Chip Pin Mapping

| Signal | NNC6521-1 | NNC6521-2 |
|--------|-----------|-----------|
| MOSI | PA7 | PB15 |
| CSN | PA4 | PB12 |
| SCLK | PA5 | PB13 |
| MISO | PA6 | PB14 |
| CHIP_EN | PC5 | PC7 |
| INTB | PC4 | PC6 |

### 2.3 Peripherals
- **Debug UART**: USART1
- **Button K1**: PC0 (active low)

### 2.4 NNC6521 Clock
- **PCLK**: 2 MHz (PCLK_DIV_1)
- **Half-wave clock cycles** = PCLK / (2 × target_frequency)

---

## 3. Waveform System Design

### 3.1 Nine Waveform Specifications

| ID | Name | Current (mA) | Freq (Hz) | Pulse Width (μs) | Type | Description |
|----|------|-------------|-----------|-------------------|------|-------------|
| 1 | Power Smooth | 3 ~ 8 | 50 | 300 | Symmetric Square | Strong smoothing |
| 2 | Burst Train | 3 ~ 8 | 50 (burst 10 Hz) | 300 | Burst Train | Burst pulse train |
| 3 | Gentle Smooth | 2 ~ 6 | 35 | 300 | Symmetric Square | Gentle smoothing |
| 4 | Deep Sculpt | 3 ~ 8 | 40 ~ 50 | 4 kHz carrier | Balanced Square | Deep sculpting |
| 5 | Soft Sculpt | 3 ~ 8 | 30 ~ 40 | 2 ~ 4 kHz | Sine | Soft sculpting |
| 6 | Circulation Sculpt | 2 ~ 6 | 2 ~ 10 | 4 kHz carrier | Balanced Sine | Circulation sculpting |
| 7 | Smooth & Firm | 1 ~ 5 | 80 ~ 100 | 400 | Triangle | Smooth & firm |
| 8 | Lymphatic Drainage | 1 ~ 4 | 5 | 450 | Low-freq Sine | Lymphatic drainage |
| 9 | Soothing Ending | 1 ~ 3 | 10 | — | Sine | Soothing ending |

### 3.2 Current Mapping Rule

User controls current intensity via percentage (0 ~ 100%):

```
actual_current = min_current + (max_current - min_current) × percent / 100
```

**Example**: Waveform 1 (3~8 mA), percent = 50 → 3 + (8 - 3) × 50 / 100 = 5.5 mA

### 3.3 Waveform Generation Method Selection

NNC6521 supports three waveform generation methods:

| Method | Description | Use Case |
|--------|-------------|----------|
| Preloaded | Built-in sine/pulse/triangle waves via register selection | Simple standard waveforms |
| Customized SPI | Normalized float array written via SPI, output by DAC | Complex/non-standard waveforms |
| Amplitude Modulation | Envelope + carrier modulation output | Carrier-modulated waveforms |

### 3.4 Sample Point Count Selection

| Waveform ID | Points | Rationale |
|-------------|--------|-----------|
| 1, 2, 3, 7, 8, 9 | 64 | Low-frequency / simple waveforms |
| 4, 5, 6 | 128 | High-frequency / complex waveforms |

---

## 4. Detailed Waveform Implementation

### 4.1 Waveform 1: Power Smooth

- **Method**: Preloaded pulse (WAVEFORM_PULSE)
- **Type**: Symmetric square wave
- **Frequency**: 50 Hz
- **Pulse Width**: 300 μs

**Register Calculation**:
- Half-wave clock: 2,000,000 / (2 × 50) = **20,000**
- Silent time: 300 × 10⁻⁶ × 2,000,000 = **600** cycles
- Rest time: 0

```c
nnc6521_preloaded_waveform(chip_id, CH0, WAVEFORM_PULSE, 64, ci,
                           20000, 20000, 600, 0);
```

---

### 4.2 Waveform 2: Burst Train

- **Method**: Customized SPI (WAVEFORM_SPI)
- **Type**: Burst pulse train
- **Frequency**: 50 Hz (burst freq 10 Hz)
- **Pulse Width**: 300 μs

**Implementation**: 64-point custom pulse array (first 32 points = 1.0, last 32 = 0.0) for burst on/off effect.

**Register Calculation**:
- Half-wave clock: **20,000**
- Silent time: **600** cycles

```c
nnc6521_customized_waveform(chip_id, CH0, 64, burst_pulse_64,
                            max_current, 20000, 20000, 600, 0, 0);
```

---

### 4.3 Waveform 3: Gentle Smooth

- **Method**: Preloaded pulse (WAVEFORM_PULSE)
- **Type**: Symmetric square wave
- **Frequency**: 35 Hz
- **Pulse Width**: 300 μs

**Register Calculation**:
- Half-wave clock: 2,000,000 / (2 × 35) = **28,571**
- Silent time: **600** cycles

```c
nnc6521_preloaded_waveform(chip_id, CH0, WAVEFORM_PULSE, 64, ci,
                           28571, 28571, 600, 0);
```

---

### 4.4 Waveform 4: Deep Sculpt

- **Method**: Customized SPI (WAVEFORM_SPI)
- **Type**: Balanced square wave (4 kHz carrier)
- **Frequency**: 40 ~ 50 Hz
- **Pulse Width**: 4 kHz carrier

**Implementation**: 128-point custom pulse array implementing 4 kHz carrier modulated by 40~50 Hz envelope.

**Register Calculation**:
- Half-wave clock (50 Hz): **20,000**
- Silent time (4 kHz): 2,000,000 / (2 × 4000) = **250** cycles

```c
nnc6521_customized_waveform(chip_id, CH0, 128, deep_sculpt_pulse_128,
                            max_current, 20000, 20000, 250, 0, 0);
```

---

### 4.5 Waveform 5: Soft Sculpt

- **Method**: Customized SPI (WAVEFORM_SPI)
- **Type**: Sine wave (2 ~ 4 kHz carrier)
- **Frequency**: 30 ~ 40 Hz

**Implementation**: 128-point sine array, the sine wave itself acts as the 30~40 Hz fundamental.

**Register Calculation**:
- Half-wave clock (40 Hz): 2,000,000 / (2 × 40) = **25,000**
- Silent time: 0 (continuous sine)

```c
nnc6521_customized_waveform(chip_id, CH0, 128, normalized_sine_waveform_128,
                            max_current, 25000, 25000, 0, 0, 1);
```

---

### 4.6 Waveform 6: Circulation Sculpt

- **Method**: Amplitude Modulation
- **Type**: Balanced sine (4 kHz carrier)
- **Frequency**: 2 ~ 10 Hz

**Implementation**: 64-point sine envelope array with 4 kHz carrier.

**Register Calculation**:
- Carrier ALT_LIM: 2,000,000 / (2 × 4000) = **250**
- Envelope interval (10 Hz): 2,000,000 / (64 × 10) = **3,125**

```c
nnc6521_amplitude_modulation(chip_id, CH0, 64, normalized_sine_waveform_64,
                             max_current, 250, 0, 3125);
```

---

### 4.7 Waveform 7: Smooth & Firm

- **Method**: Preloaded triangle (WAVEFORM_TRIANGLE)
- **Type**: Triangle wave
- **Frequency**: 80 ~ 100 Hz
- **Pulse Width**: 400 μs

**Register Calculation**:
- Half-wave clock (100 Hz): 2,000,000 / (2 × 100) = **10,000**
- Silent time: 400 × 10⁻⁶ × 2,000,000 = **800** cycles

```c
nnc6521_preloaded_waveform(chip_id, CH0, WAVEFORM_TRIANGLE, 64, ci,
                           10000, 10000, 800, 0);
```

---

### 4.8 Waveform 8: Lymphatic Drainage

- **Method**: Customized SPI (WAVEFORM_SPI)
- **Type**: Low-frequency sine
- **Frequency**: 5 Hz
- **Pulse Width**: 450 μs

**Implementation**: 64-point sine array at 5 Hz.

**Register Calculation**:
- Half-wave clock: 2,000,000 / (2 × 5) = **200,000**
- Silent time: 450 × 10⁻⁶ × 2,000,000 = **900** cycles

```c
nnc6521_customized_waveform(chip_id, CH0, 64, normalized_sine_waveform_64,
                            max_current, 200000, 200000, 900, 0, 1);
```

---

### 4.9 Waveform 9: Soothing Ending

- **Method**: Preloaded sine (WAVEFORM_SINE)
- **Type**: Sine wave
- **Frequency**: 10 Hz

**Register Calculation**:
- Half-wave clock: 2,000,000 / (2 × 10) = **100,000**
- Silent time: 0 (continuous sine)

```c
nnc6521_preloaded_waveform(chip_id, CH0, WAVEFORM_SINE, 64, ci,
                           100000, 100000, 0, 0);
```

---

## 5. NNC6521 Driver API Reference

### 5.1 Initialization

```c
nnc6521_gpio_init();                    // Initialize GPIO pins
nnc6521_init(NNC6521_CHIP_1);           // Initialize chip 1
nnc6521_init(NNC6521_CHIP_2);           // Initialize chip 2
```

### 5.2 Preloaded Waveform Output

```c
void nnc6521_preloaded_waveform(
    uint8_t  chip_id,               // Chip ID (0 or 1)
    uint8_t  u8_Channel,            // Channel (CH0=0, CH1=1)
    uint8_t  u8_Waveform,           // Waveform type (SINE/PULSE/TRIANGLE)
    uint8_t  u8_PointNum,           // Sample points (64 or 128)
    uint8_t  u8_CI,                 // Current drive strength
    uint16_t u32_Positive_Interval, // Positive half-wave clock cycles
    uint16_t u32_Negative_Interval, // Negative half-wave clock cycles
    uint32_t u32_Silent_Time,       // Silent time (clock cycles)
    uint16_t u16_Rest_Time          // Rest time (clock cycles)
);
```

### 5.3 Customized SPI Waveform Output

```c
void nnc6521_customized_waveform(
    uint8_t  chip_id,                    // Chip ID
    uint8_t  u8_Channel,                 // Channel
    uint8_t  u8_PointNum,                // Sample points
    float   *f_Normalized_array,         // Normalized array (0.0 ~ 1.0)
    uint32_t u32_Max_current,            // Max current (mA)
    uint16_t u32_Positive_Interval,      // Positive half-wave clock cycles
    uint16_t u32_Negative_Interval,      // Negative half-wave clock cycles
    uint32_t u32_Silent_Time,            // Silent time
    uint16_t u16_Rest_Time,              // Rest time
    uint8_t  u8_Asymmetric_Symmetric     // 0=asymmetric, 1=symmetric
);
```

### 5.4 Amplitude Modulation Waveform Output

```c
void nnc6521_amplitude_modulation(
    uint8_t  chip_id,                    // Chip ID
    uint8_t  u8_Channel,                 // Channel
    uint8_t  u8_PointNum,                // Envelope sample points
    float   *f_Normalized_Envelope_array, // Normalized envelope array
    uint32_t u32_Max_current,            // Max current (mA)
    uint16_t u16_Carrier,                // Carrier half-wave clock cycles
    uint32_t u32_Silent_Time,            // Silent time
    uint16_t u16_Interval                // Envelope interval clock cycles
);
```

### 5.5 AWG Enable/Disable

```c
void nnc6521_awg_enable_disable(
    uint8_t chip_id,                // Chip ID
    uint8_t AWG_ChannelNum,         // Channel
    uint8_t Enable_Disable          // 1=enable, 0=disable
);
```

### 5.6 SPI Communication Protocol

**SPI Write Protocol**:
```
4 bytes: [addr, cmd, data, 0x00]
- Normal register: cmd = 0x80
- Waveform register: cmd = 0xC0
- Data in byte 2, byte 3 fixed to 0x00
```

**SPI Read Protocol**:
```
3 bytes: [addr, cmd, dummy]
- Normal register: cmd = 0x00
- Waveform register: cmd = 0x40
- Data returned in byte 3
```

**Bit-Bang Timing**:
```
NOP delays before/after each clock edge (~55ns @ 72MHz), meeting NNC6521 timing requirements.
```

---

## 6. Waveform Configuration Module API

### 6.1 Configuration Structure

```c
typedef struct {
    uint8_t     id;              // Waveform ID (1~9)
    const char *name;            // Waveform name
    const char *description;     // Waveform description
    uint32_t    min_current;     // Minimum current (mA)
    uint32_t    max_current;     // Maximum current (mA)
    uint16_t    frequency;       // Main frequency (Hz)
    uint16_t    pulse_width_us;  // Pulse width (μs)
    uint8_t     waveform_type;   // Waveform type enum
    uint8_t     gen_method;      // Generation method (0=preloaded, 1=custom SPI, 2=AM)
    uint8_t     point_num;       // Sample point count
    uint16_t    half_wave_clk;   // Half-wave clock cycles
    uint32_t    silent_time;     // Silent time
    uint16_t    rest_time;       // Rest time
    uint16_t    carrier_freq;    // Carrier frequency (AM only)
    uint16_t    am_interval;     // Envelope interval (AM only)
    float      *waveform_data;   // Waveform data pointer
} waveform_config_t;
```

### 6.2 Core Functions

```c
/* Apply waveform to NNC6521 by ID and current percentage */
void waveform_apply(uint8_t chip_id, uint8_t channel,
                    uint8_t waveform_id, uint8_t percent);

/* Calculate actual current from percentage */
uint32_t waveform_calc_current(uint8_t waveform_id, uint8_t percent);

/* Get waveform configuration by ID */
const waveform_config_t* waveform_get_config(uint8_t waveform_id);
```

---

## 7. Test Verification Plan

### 7.1 Test Flow

1. Power on → system initializes NNC6521 dual chips
2. Default: load Waveform 1 (Power Smooth), current 50%
3. Each K1 press cycles to next waveform (1→2→...→9→1)
4. UART prints current waveform info (name, current, frequency, type)

### 7.2 UART Output Format

```
========================================
Waveform #1: Power Smooth
  Current: 55 mA (50%)
  Frequency: 50 Hz
  Type: Preloaded Pulse
========================================
```

### 7.3 Verification Checklist

| Check Item | Expected Result |
|------------|----------------|
| K1 button switching | 9 waveforms cycle correctly |
| UART output | Print waveform info on each switch |
| Waveform output | NNC6521 CH0 outputs correct waveform |
| Current range | Actual current within specified range |
| Frequency accuracy | Actual frequency within specified range |

---

## 8. Change Log

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| V1.0 | 2026-07-01 | Engineering Team | Initial version, define 9 waveform specs and implementation |
| V1.1 | 2026-07-14 | Engineering Team | Added SPI communication protocol section (write/read protocol, bit-bang timing) |
| V1.2 | 2026-07-30 | Engineering Team | Reduced all waveform current ranges by 10x (e.g. 30~80 mA → 3~8 mA) |
