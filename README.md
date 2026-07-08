# verify-nnc6521-demo

NNC6521 双芯片波形驱动验证工程 — 基于 RT-Thread + STM32F103RCT6

## 项目概述

本工程用于验证 NNC6521 模拟前端芯片的波形发生功能，通过软件 SPI 驱动两颗 NNC6521 芯片，支持 9 种预定义波形输出和按键切换。

## 硬件平台

| 项目 | 参数 |
|------|------|
| 主控 | STM32F103RCT6 (72 MHz, 256KB Flash, 48KB SRAM) |
| 外部晶振 | 16 MHz HSE (DIV2 → PLL ×9 → 72 MHz) |
| 模拟前端 | 2× NNC6521 (QFN32, 双通道恒流源驱动) |
| NNC6521 时钟 | 内部 2 MHz PCLK（可通过寄存器 1/2/4/8/16/32/64/128 分频） |
| RTOS | RT-Thread 5.1.0 |
| SPI | 软件 SPI (GPIO 位操作，CPOL=0, CPHA=0, MSB 先传) |

## 引脚映射

### NNC6521-1

| STM32 | 功能 | NNC6521-1 |
|-------|------|-----------|
| PA4 | SPI1_CS | CSN |
| PA5 | SPI1_SCLK | SCLK |
| PA6 | SPI1_MISO | MISO |
| PA7 | SPI1_MOSI | MOSI |
| PC4 | INTB | INTB |
| PC5 | CHIP_EN | CHIP_EN |

### NNC6521-2

| STM32 | 功能 | NNC6521-2 |
|-------|------|-----------|
| PB12 | SPI2_CS | CSN |
| PB13 | SPI2_SCLK | SCLK |
| PB14 | SPI2_MISO | MISO |
| PB15 | SPI2_MOSI | MOSI |
| PC6 | INTB | INTB |
| PC7 | CHIP_EN | CHIP_EN |

### 其他引脚

| 引脚 | 功能 | 配置 |
|------|------|------|
| PC0 | K1 按键 | 上拉输入，低电平有效 |
| PB0/PB1/PB10/PB11 | 继电器 RAY1~RAY4 | 推挽输出，高电平导通 |
| PA9/PA10 | USART1 (调试串口) | 115200-8N1 |

## 波形配置

默认应用 Waveform 1 (Power Smooth)，通过 K1 按键循环切换 9 种波形：

| ID | 名称 | 频率 | 电流范围 | 类型 |
|----|------|------|---------|------|
| 1 | Power Smooth | 50 Hz | 30~80 mA | 方波 (预加载) |
| 2 | Burst Train | 50 Hz | 30~80 mA | 突发脉冲 (SPI) |
| 3 | Gentle Smooth | 35 Hz | 20~60 mA | 方波 (预加载) |
| 4 | Deep Sculpt | 50 Hz | 30~80 mA | 平衡方波 (SPI) |
| 5 | Soft Sculpt | 40 Hz | 30~80 mA | 正弦波 (SPI) |
| 6 | Circulation Sculpt | 10 Hz | 20~60 mA | 幅度调制 (AM) |
| 7 | Smooth & Firm | 100 Hz | 15~50 mA | 三角波 (预加载) |
| 8 | Lymphatic Drainage | 5 Hz | 15~40 mA | 正弦波 (SPI) |
| 9 | Soothing Ending | 10 Hz | 10~30 mA | 正弦波 (SPI) |

## 目录结构

```
verify-nnc6521-demo/
├── applications/
│   ├── main.c                          # 主入口，波形切换逻辑
│   ├── macBSP/
│   │   ├── Inc/
│   │   │   ├── nnc6521.h               # NNC6521 驱动 API 头文件
│   │   │   ├── nnc6521_reg.h           # 寄存器地址与位域定义
│   │   │   └── nnc6521_waveform_config.h # 波形配置头文件
│   │   └── Src/
│   │       ├── nnc6521_drv.c           # NNC6521 高级驱动（波形配置、LOD、SCD）
│   │       ├── nnc6521_spi.c           # 软件 SPI 实现（GPIO 位操作）
│   │       ├── nnc6521_waveform.c      # 预加载波形数据数组
│   │       └── nnc6521_waveform_config.c # 9 种波形配置与应用函数
│   └── macSYS/
│       ├── Inc/
│       │   ├── bsp_sys.h
│       │   ├── bsp_typedef.h
│       │   └── rtt_system_work.h
│       └── Src/
│           ├── bsp_sys.c
│           ├── bsp_typedef.c
│           └── rtt_system_work.c       # 系统定时器回调
├── cubemx/                             # CubeMX 生成的代码
│   ├── Inc/main.h                      # 引脚定义 (Key1, RAY1~RAY4)
│   └── Src/main.c                      # GPIO 初始化 (弱定义)
├── docs/
│   ├── 开发板硬件资源描述.md
│   ├── NNC6521-waveform-design-spec.md
│   └── NNC6521波形设计需求文档.md
├── drivers/                            # RT-Thread BSP 驱动
├── rt-thread/                          # RT-Thread 内核源码
└── rtconfig.h                          # RT-Thread 配置
```

## 编译与烧录

1. 使用 Keil MDK 5.23+ 打开工程文件
2. 编译：Build → Rebuild All
3. 烧录：通过 J-Link 下载到 STM32F103RCT6

## 调试串口输出

上电后串口输出示例：

```
\ | /
- RT - Thread Operating System
 / | \ 5.1.0 build Jul 2 2026 17:59:37
 2006 - 2024 Copyright by RT-Thread team
PRINTF:0. sysTimer initialize succeed!

========================================
Waveform #1: Power Smooth
 Current: 55 mA (50%)
 Frequency: 50 Hz
 Description: Strong smoothing symmetric square wave
========================================
```

按 K1 键切换波形，串口输出新波形信息。

## 与 v10 正式版的差异

| 项目 | verify-nnc6521-demo（本工程） | microcurrent-beauty-device-v10 |
|------|------|------|
| 外部晶振 | 16 MHz (HSE DIV2) | 8 MHz (HSE DIV1) |
| 系统时钟 | 72 MHz | 72 MHz |
| 波形切换 | K1 按键（PC0）循环切换 | 软件协议命令切换 |
| GPIO 引脚 | Chip1: PA4~PA7/PC4~PC5 / Chip2: PB12~PB15/PC6~PC7 | Chip1: PC7~PC9/PA8/PA11~PA12 / Chip2: PB11~PB15/PC6 |
| NTC 温控 | 无 | 有（NTC 传感器 + PID 控制） |
| 通讯协议 | 无 | 串口协议（protocol.c） |

> **注意**：NNC6521 使用内部 2 MHz 振荡器，与外部晶振无关，因此两个工程的波形时序参数完全一致。

## 关键 API

```c
// 芯片初始化
nnc6521_gpio_init();                    // 初始化所有 GPIO
nnc6521_init(NNC6521_CHIP_1);           // 芯片 1 上电（CHIP_EN 拉高后等待 500ms）
nnc6521_init(NNC6521_CHIP_2);           // 芯片 2 上电

// 波形输出
waveform_apply(chip_id, channel, waveform_id, percent);  // 应用指定波形
nnc6521_preloaded_waveform(...);        // 预加载波形（正弦/脉冲/三角）
nnc6521_customized_waveform(...);       // SPI 自定义波形
nnc6521_amplitude_modulation(...);      // 幅度调制波形

// 波形控制
nnc6521_awg_enable_disable(chip_id, channel, enable);  // 使能/禁用 AWG

// 检测模块
nnc6521_lod_init(...);                  // 导联脱落检测初始化
nnc6521_scd_init(...);                  // 短路检测初始化
nnc6521_clear_lod_int(chip_id);         // 清除 LOD 中断
nnc6521_clear_scd_int(chip_id);         // 清除 SCD 中断
```

## 注意事项

1. **CHIP_EN 延时**：芯片使能后需等待 ≥500ms 才能进行 SPI 通信
2. **继电器**：刺激输出前需确保 PB0/PB1/PB10/PB11 继电器已导通（高电平）
3. **OTP 读取**：VPP 选择按钮需拨至 1.8V 才能读取 OTP 校准数据
4. **示波器测量**：使用双探头 + Math 功能 (CH1-CH2) 观察双向波形

## 波形技术细节

### PCLK 分频策略

低频波形（5 Hz、10 Hz）的半波时钟周期会超出 16-bit 寄存器范围，需要降低 PCLK：

| 波形 ID | 频率 | PCLK 分频 | 实际 PCLK | half_wave_clk |
|---------|------|----------|-----------|---------------|
| 1~5, 7 | 35~100 Hz | ÷1 | 2 MHz | 10000~28571 |
| 6, 9 | 10 Hz | ÷8 | 250 kHz | 12500 |
| 8 | 5 Hz | ÷16 | 125 kHz | 12500 |

### 安全检测模块

本工程集成了 NNC6521 的两个硬件保护模块：

- **LOD（Lead-Off Detection）**：检测电极与皮肤接触不良，可触发中断
- **SCD（Short-Circuit Detection）**：检测输出短路，可自动停止波形输出

## 许可证

Apache-2.0
