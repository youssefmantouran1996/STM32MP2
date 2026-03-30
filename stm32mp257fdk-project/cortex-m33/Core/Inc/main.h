#ifndef MAIN_H
#define MAIN_H

#include "stm32mp2xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void Error_Handler(void);

/* ── User LED (STM32MP257FDK — adjust to match schematic) ───────────────── */
#define USER_LED_PIN        GPIO_PIN_13
#define USER_LED_GPIO_PORT  GPIOH

/* ── User button ────────────────────────────────────────────────────────── */
#define USER_BTN_PIN        GPIO_PIN_0
#define USER_BTN_GPIO_PORT  GPIOA

/* ── UART4 debug console (PD6=TX, PD5=RX, AF3) ──────────────────────────── */
#define LOG_UART_INSTANCE   UART4
#define LOG_UART_TX_PIN     GPIO_PIN_6
#define LOG_UART_RX_PIN     GPIO_PIN_5
#define LOG_UART_GPIO_PORT  GPIOD
#define LOG_UART_AF         GPIO_AF3_UART4
#define LOG_UART_BAUDRATE   115200U

#ifdef __cplusplus
}
#endif
#endif /* MAIN_H */
