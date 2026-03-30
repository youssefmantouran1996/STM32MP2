/**
 * @file  system_stm32mp2xx.c
 * @brief Minimal SystemInit for Cortex-M33 remoteproc use case.
 *
 * On the STM32MP257FDK the M33 is started by the A35 (via remoteproc)
 * after TF-A and U-Boot have already configured the clock tree and DDR.
 * There is nothing for the M33 to do in SystemInit; all peripheral clocks
 * are enabled on demand through the HAL RCC macros.
 *
 * For bare-metal JTAG bring-up (without remoteproc), add clock init here.
 */

#include "system_stm32mp2xx.h"

uint32_t SystemCoreClock = 200000000UL;  /* Default: 200 MHz (set by TF-A) */

void SystemInit(void) {
    /*
     * Clock tree is managed by TF-A / U-Boot.
     * Optionally enable the instruction cache here for performance.
     */
}

void SystemCoreClockUpdate(void) {
    /* Update SystemCoreClock from RCC registers if needed. */
}
