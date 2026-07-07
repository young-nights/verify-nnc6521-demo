/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-30     RT-Thread    first version
 */

#include <rtthread.h>

#include "bsp_sys.h"
#include "nnc6521.h"
#include "nnc6521_waveform_config.h"


#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/**
  * @brief  应用程序主入口
  *
  * 主流程：
  * 1. HAL 初始化（复位外设、Flash 接口、SysTick）
  * 2. 系统时钟配置
  * 3. GPIO 和串口外设初始化
  * 4. NNC6521 双芯片初始化（GPIO + CHIP_EN 上电序列）
  * 5. 应用默认波形并输出波形信息到串口
  * 6. 主循环：K1 按键检测 → 波形切换 → 串口输出
  *
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU 配置 --------------------------------------------------------*/

  /* 复位所有外设，初始化 Flash 接口和 SysTick */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟 */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* 初始化所有配置的外设 */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* 初始化 NNC6521 GPIO 引脚和双芯片上电 */
  nnc6521_gpio_init();
  nnc6521_init(NNC6521_CHIP_1);   /* 芯片 1 上电初始化（CHIP_EN = PC5） */
  nnc6521_init(NNC6521_CHIP_2);   /* 芯片 2 上电初始化（CHIP_EN = PC7） */

  /* Enable analog output stage (VDAC + driver amp) for CH1 on both chips */
  nnc6521_analog_enable(NNC6521_CHIP_1, WAVEFORM_GEN_CH0);
  nnc6521_analog_enable(NNC6521_CHIP_2, WAVEFORM_GEN_CH0);

  /* 波形切换状态变量 */
  uint8_t current_waveform_id = 1;  /* 当前波形编号，从 Waveform 1 开始 */
  uint8_t current_percent = WAVEFORM_DEFAULT_PCT;  /* 默认电流百分比 */
  uint8_t k1_last_state = 1;       /* K1 上次状态：高电平 = 释放 */
  uint8_t k1_pressed = 0;          /* K1 按下标志 */

  /* 应用初始波形 */
  waveform_apply(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
                 current_waveform_id, current_percent);

  /* 通过串口输出初始波形信息 */
  {
      const waveform_config_t *cfg = waveform_get_config(current_waveform_id);
      if (cfg != NULL) {
          uint32_t actual_ma = waveform_calc_current(current_waveform_id, current_percent);
          rt_kprintf("\r\n========================================\r\n");
          rt_kprintf("Waveform #%d: %s\r\n", cfg->id, cfg->name);
          rt_kprintf("  Current: %d mA (%d%%)\r\n", actual_ma, current_percent);
          rt_kprintf("  Frequency: %d Hz\r\n", cfg->frequency);
          rt_kprintf("  Description: %s\r\n", cfg->description);
          rt_kprintf("========================================\r\n");
      }
  }

  /* USER CODE END 2 */

  /* 主循环 */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* K1 按键检测（PC0，低电平有效）：下降沿检测 + 50ms 去抖 */
    uint8_t k1_current = (uint8_t)HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);

    /* 下降沿检测：上次高电平 + 当前低电平 = 按键按下事件 */
    if (k1_last_state == 1 && k1_current == 0) {
        k1_pressed = 1;
    }
    k1_last_state = k1_current;

    if (k1_pressed) {
        k1_pressed = 0;

        rt_kprintf("\r\n[DBG] Key pressed! Current=#%d\r\n", current_waveform_id);

        /* 切换到下一个波形（1 → 2 → ... → 9 → 1 循环） */
        current_waveform_id++;
        if (current_waveform_id > WAVEFORM_COUNT) {
            current_waveform_id = 1;
        }

        rt_kprintf("[DBG] Target=#%d, disabling AWG...\r\n", current_waveform_id);

        /* 切换前先禁用当前波形输出 */
        nnc6521_awg_enable_disable(NNC6521_CHIP_1, WAVEFORM_GEN_CH0, 0);

        rt_kprintf("[DBG] AWG disabled, applying waveform...\r\n");

        /* 应用新波形 */
        waveform_apply(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
                       current_waveform_id, current_percent);

        rt_kprintf("[DBG] Waveform #%d applied OK\r\n", current_waveform_id);

        /* 通过串口输出新波形信息 */
        {
            const waveform_config_t *cfg = waveform_get_config(current_waveform_id);
            if (cfg != NULL) {
                uint32_t actual_ma = waveform_calc_current(current_waveform_id, current_percent);
                rt_kprintf("\r\n========================================\r\n");
                rt_kprintf("Waveform #%d: %s\r\n", cfg->id, cfg->name);
                rt_kprintf("  Current: %d mA (%d%%)\r\n", actual_ma, current_percent);
                rt_kprintf("  Frequency: %d Hz\r\n", cfg->frequency);
                rt_kprintf("  Description: %s\r\n", cfg->description);
                rt_kprintf("========================================\r\n");
            }
        }
    }

    /* 50ms 去抖延时 */
    rt_thread_mdelay(50);
  }
  /* USER CODE END 3 */
}
