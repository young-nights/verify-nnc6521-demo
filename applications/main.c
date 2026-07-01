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
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* Initialize NNC6521 GPIO and both chips */
  nnc6521_gpio_init();
  nnc6521_init(NNC6521_CHIP_1);
  nnc6521_init(NNC6521_CHIP_2);

  /* Waveform switching state */
  uint8_t current_waveform_id = 1;  /* Start with waveform 1 */
  uint8_t current_percent = WAVEFORM_DEFAULT_PCT;
  uint8_t k1_last_state = 1;       /* PC0: high = released */
  uint8_t k1_pressed = 0;

  /* Apply initial waveform */
  waveform_apply(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
                 current_waveform_id, current_percent);

  /* Print initial waveform info */
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

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* K1 button detection (PC0, active low) with debouncing */
    uint8_t k1_current = (uint8_t)HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);

    if (k1_last_state == 1 && k1_current == 0) {
        /* Falling edge detected -> button press */
        k1_pressed = 1;
    }
    k1_last_state = k1_current;

    if (k1_pressed) {
        k1_pressed = 0;

        /* Cycle to next waveform (1 -> 2 -> ... -> 9 -> 1) */
        current_waveform_id++;
        if (current_waveform_id > WAVEFORM_COUNT) {
            current_waveform_id = 1;
        }

        /* Disable current waveform before switching */
        nnc6521_awg_enable_disable(NNC6521_CHIP_1, WAVEFORM_GEN_CH0, 0);

        /* Apply new waveform */
        waveform_apply(NNC6521_CHIP_1, WAVEFORM_GEN_CH0,
                       current_waveform_id, current_percent);

        /* Print waveform info */
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

    /* Debounce delay */
    rt_thread_mdelay(50);
  }
  /* USER CODE END 3 */
}
