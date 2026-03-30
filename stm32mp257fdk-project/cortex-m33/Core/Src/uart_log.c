/**
 * @file  uart_log.c
 * @brief UART4 logging implementation for the Cortex-M33.
 */

#include "uart_log.h"
#include "stm32mp2xx_hal.h"
#include <stdio.h>
#include <string.h>

/* ── Module-private state ────────────────────────────────────────────────── */
static UART_HandleTypeDef s_huart4;
static uint8_t s_init_done = 0;

/* ── Public API ─────────────────────────────────────────────────────────── */

void uart_log_init(void) {
    s_huart4.Instance          = UART4;
    s_huart4.Init.BaudRate     = 115200;
    s_huart4.Init.WordLength   = UART_WORDLENGTH_8B;
    s_huart4.Init.StopBits     = UART_STOPBITS_1;
    s_huart4.Init.Parity       = UART_PARITY_NONE;
    s_huart4.Init.Mode         = UART_MODE_TX_RX;
    s_huart4.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    s_huart4.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&s_huart4) == HAL_OK) {
        s_init_done = 1;
        uart_log_writeln("\r\n[UART] Log initialised — STM32MP257FDK M33");
    }
}

void uart_log_write(const char *str) {
    if (!s_init_done || str == NULL) {
        return;
    }
    HAL_UART_Transmit(&s_huart4, (const uint8_t *)str,
                      (uint16_t)strlen(str), HAL_MAX_DELAY);
}

void uart_log_writeln(const char *str) {
    uart_log_write(str);
    uart_log_write("\r\n");
}

void uart_log_printf(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_log_write(buf);
}

void uart_log_hex32(uint32_t val) {
    char buf[11];
    snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)val);
    uart_log_write(buf);
}

/**
 * @brief HAL UART MSP init callback — configures GPIO for UART4.
 *
 * Called automatically by HAL_UART_Init().
 *
 * Pin mapping (STM32MP257FDK default debug UART):
 *   PD6  → UART4_TX  (AF3)
 *   PD5  → UART4_RX  (AF3)
 *
 * If your board uses different pins, adjust Pin and Alternate below.
 * AF3 maps to UART4 on PD5/PD6 per the STM32MP257 datasheet Table 17.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance == UART4) {
        __HAL_RCC_UART4_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        gpio.Pin       = GPIO_PIN_6 | GPIO_PIN_5;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF3_UART4;
        HAL_GPIO_Init(GPIOD, &gpio);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART4) {
        __HAL_RCC_UART4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_6 | GPIO_PIN_5);
    }
}
