/**
 * @file    main.c
 * @brief   STM32MP257FDK — Cortex-M33 firmware entry point.
 *
 * The Cortex-M33 runs as a coprocessor alongside the Cortex-A35.
 * OpenSTLinux on the A35 loads this firmware via remoteproc, which also
 * populates the OpenAMP resource table (shared/ipc/resource_table.c) with
 * the DDR addresses of the vring ring buffers before starting this core.
 *
 * Startup sequence
 * ────────────────
 * 1. HAL + GPIO init
 * 2. UART4 log console (115200 baud on PD6/PD5)
 * 3. IPC init: reads vring addresses from resource table, sends RPMsg NS
 *    announcement so Linux creates /dev/ttyRPMSG0
 * 4. Main loop: toggle LED, poll IPC for commands from A35
 *
 * IPC protocol (see ipc_handler.h for full documentation):
 *   A35 → M33  5-byte command:   [0xAA][CMD][AL][AH][0x55]
 *   M33 → A35  8-byte response:  [0xBB][CMD][AL][AH][ST][T0][T1][0x55]
 */

#include "main.h"
#include "fw_version.h"
#include "uart_log.h"
#include "ipc_handler.h"
#include <stdint.h>

/* ── Firmware version (reported via IPC_CMD_GET_VERSION) ─────────────────── */
const FwVersion g_fw_version = {
    .major           = FW_VERSION_MAJOR,
    .minor           = FW_VERSION_MINOR,
    .patch           = FW_VERSION_PATCH,
    .reserved        = 0U,
    .build_timestamp = FW_BUILD_TIMESTAMP,
};

/* ── Private function prototypes ──────────────────────────────────────────── */
static void SystemClock_Config(void);
static void GPIO_Init(void);

/* ── LED blink interval (can be changed by IPC_CMD_LED_BLINK) ──────────── */
volatile uint32_t g_blink_ms = 500U;

/* ── Application entry point ─────────────────────────────────────────────── */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    uart_log_init();
    LOG_INFO("STM32MP257FDK M33 firmware v%u.%u.%u starting",
             (unsigned)g_fw_version.major,
             (unsigned)g_fw_version.minor,
             (unsigned)g_fw_version.patch);
    LOG_INFO("HAL tick = %lu ms", (unsigned long)HAL_GetTick());

    IPC_Init();

    uint32_t last_toggle = HAL_GetTick();

    while (1) {
        /* Freeze normal operation once OTA_PREPARE has been acknowledged.
         * remoteproc on the A35 will stop this core shortly after. */
        if (IPC_IsOtaPending()) {
            HAL_GPIO_WritePin(USER_LED_GPIO_PORT, USER_LED_PIN, GPIO_PIN_RESET);
            LOG_INFO("OTA pending — core halted, waiting for remoteproc stop");
            while (IPC_IsOtaPending()) {
                /* Spin here; do not touch shared DDR vrings */
                __WFI();
            }
        }

        /* Toggle LED at g_blink_ms interval to show the core is alive */
        uint32_t now = HAL_GetTick();
        if ((now - last_toggle) >= g_blink_ms) {
            HAL_GPIO_TogglePin(USER_LED_GPIO_PORT, USER_LED_PIN);
            last_toggle = now;
        }

        /* Process incoming RPMsg messages from the A35 */
        IPC_Process();
    }
}

/* ── System clock configuration ──────────────────────────────────────────── */
static void SystemClock_Config(void) {
    /*
     * On the STM32MP257FDK the clock tree is configured by TF-A/U-Boot
     * before remoteproc starts the M33.  Nothing to do here for the
     * remoteproc use case.
     *
     * For bare-metal JTAG bring-up, configure the M33 clock domain here
     * using HAL_RCC_OscConfig() and HAL_RCC_ClockConfig().
     */
}

/* ── GPIO initialisation ─────────────────────────────────────────────────── */
static void GPIO_Init(void) {
    GPIO_InitTypeDef gpio = {0};

    /* User LED on GPIOH pin 13 */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    gpio.Pin   = USER_LED_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(USER_LED_GPIO_PORT, USER_LED_PIN, GPIO_PIN_RESET);
}

/* ── HAL error handler ───────────────────────────────────────────────────── */
void Error_Handler(void) {
    __disable_irq();
    LOG_ERR("Error_Handler called — spinning");
    while (1) {
        /* Attach a JTAG debugger to inspect the call stack */
    }
}
