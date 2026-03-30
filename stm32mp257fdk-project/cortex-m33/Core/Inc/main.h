#ifndef MAIN_H
#define MAIN_H

#include "stm32mp2xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);

/* User-defined LED / button pins (adjust to match your schematic) */
#define USER_LED_PIN       GPIO_PIN_13
#define USER_LED_GPIO_PORT GPIOH
#define USER_BTN_PIN       GPIO_PIN_0
#define USER_BTN_GPIO_PORT GPIOA

#ifdef __cplusplus
}
#endif
#endif /* MAIN_H */
