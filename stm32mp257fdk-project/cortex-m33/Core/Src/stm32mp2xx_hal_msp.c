/**
 * @file  stm32mp2xx_hal_msp.c
 * @brief HAL low-level hardware initialisation callbacks (MSP = MCU Support Package).
 *
 * HAL_Init() calls HAL_MspInit().
 * Each HAL_<Peripheral>_Init() call triggers HAL_<Peripheral>_MspInit().
 * UART4 GPIO init lives in uart_log.c (HAL_UART_MspInit) to keep it
 * co-located with the UART driver code.
 */

#include "main.h"

/**
 * @brief Global MSP initialisation — called by HAL_Init().
 *        Configure NVIC priority grouping here.
 */
void HAL_MspInit(void) {
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}
