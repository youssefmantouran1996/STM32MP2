#ifndef STM32MP2XX_HAL_CONF_H
#define STM32MP2XX_HAL_CONF_H

/* ── Enable used HAL modules ──────────────────────────────────────────────── */
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_IPCC_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
/* Uncomment additional modules as your project requires:
#define HAL_I2C_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
*/

/* ── Oscillator values (must be defined before HAL includes) ──────────────── */
#define HSI_VALUE           64000000UL   /* 64 MHz internal oscillator */
#define HSE_VALUE           24000000UL   /* 24 MHz external oscillator */
#define LSI_VALUE              32000UL   /* 32 kHz low-speed internal */
#define LSE_VALUE              32768UL   /* 32.768 kHz low-speed external */
#define HSE_STARTUP_TIMEOUT     100UL   /* ms timeout for HSE start */
#define LSE_STARTUP_TIMEOUT    5000UL   /* ms timeout for LSE start */

/* ── SysTick tick rate ─────────────────────────────────────────────────────── */
#define TICK_INT_PRIORITY   0x0FUL

/* assert_param: define as no-op unless USE_FULL_ASSERT is enabled */
#ifdef USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

#include "stm32mp2xx_hal_def.h"

#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32mp2xx_hal_rcc.h"
  #include "stm32mp2xx_hal_rcc_ex.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32mp2xx_hal_gpio.h"
  #include "stm32mp2xx_hal_gpio_ex.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32mp2xx_hal_cortex.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32mp2xx_hal_pwr.h"
  #include "stm32mp2xx_hal_pwr_ex.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32mp2xx_hal_uart.h"
  #include "stm32mp2xx_hal_uart_ex.h"
#endif
#ifdef HAL_IPCC_MODULE_ENABLED
  #include "stm32mp2xx_hal_ipcc.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32mp2xx_hal_exti.h"
#endif

#endif /* STM32MP2XX_HAL_CONF_H */
