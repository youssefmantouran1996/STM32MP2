#ifndef STM32MP2XX_HAL_CONF_H
#define STM32MP2XX_HAL_CONF_H

/* ── Enable used HAL modules ──────────────────────────────────────────────── */
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
/* Uncomment additional modules as your project requires:
#define HAL_I2C_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
*/

/* ── HSI oscillator (adjust to match your board clock configuration) ──────── */
#define HSI_VALUE    64000000UL  /* 64 MHz */

/* ── SysTick tick rate ─────────────────────────────────────────────────────── */
#define TICK_INT_PRIORITY   0x0FUL

#include "stm32mp2xx_hal_def.h"

#endif /* STM32MP2XX_HAL_CONF_H */
