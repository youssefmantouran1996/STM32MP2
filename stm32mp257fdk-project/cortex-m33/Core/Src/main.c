/**
 * @file    main.c
 * @brief   STM32MP257FDK — Cortex-M33 firmware entry point
 *
 * The Cortex-M33 runs as a coprocessor alongside the Cortex-A35.
 * On boot, OpenSTLinux on the A35 loads this firmware via remoteproc.
 *
 * IPC with the A35 is handled through OpenAMP / RPMsg virtual channels
 * defined in shared/ipc/resource_table.h.
 */

#include "main.h"
#include <stdint.h>

/* ─── Private function prototypes ──────────────────────────────────────────── */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void IPC_Init(void);

/* ─── Application entry point ─────────────────────────────────────────────── */
int main(void) {
    /* HAL initialisation — configures SysTick, flash, etc. */
    HAL_Init();

    /* Configure system clocks */
    SystemClock_Config();

    /* Initialise peripherals */
    GPIO_Init();

    /* Initialise OpenAMP IPC with the Cortex-A35 */
    IPC_Init();

    /* ── Main loop ────────────────────────────────────────────────────────── */
    while (1) {
        /* Toggle user LED to indicate the M33 is alive */
        HAL_GPIO_TogglePin(USER_LED_GPIO_PORT, USER_LED_PIN);
        HAL_Delay(500);

        /*
         * Poll / process OpenAMP RPMsg messages from the A35.
         * Replace this with your application logic.
         */
    }
}

/* ─── System clock configuration ──────────────────────────────────────────── */
static void SystemClock_Config(void) {
    /*
     * The STM32MP257 clock tree is complex and shared with the A35.
     * In production, clock configuration is done by TF-A / U-Boot.
     * For bare-metal testing, configure only the clocks owned by the M33.
     *
     * Refer to: STM32CubeMX → Clock Configuration tab
     * and the STM32MP25 Reference Manual (RM0457), section "RCC".
     */
}

/* ─── GPIO initialisation ─────────────────────────────────────────────────── */
static void GPIO_Init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Pin   = USER_LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_GPIO_PORT, &gpio);
}

/* ─── OpenAMP IPC initialisation ──────────────────────────────────────────── */
static void IPC_Init(void) {
    /*
     * Initialise the OpenAMP / RPMsg stack.
     * The resource table in shared/ipc/resource_table.h tells the A35
     * where to find the shared memory ring buffers.
     *
     * Steps:
     *   1. Initialise the shared memory region (no-cache / non-bufferable)
     *   2. Initialise the virtual driver (VDEV) and vring descriptors
     *   3. Register RPMsg endpoints for each IPC channel
     *
     * See ST application note AN5903 and the STM32CubeMP2 OpenAMP examples.
     */
}

/* ─── HAL error handler ───────────────────────────────────────────────────── */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        /* Spin — attach debugger to inspect fault registers */
    }
}
