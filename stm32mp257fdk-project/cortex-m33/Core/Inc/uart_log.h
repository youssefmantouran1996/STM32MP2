/**
 * @file  uart_log.h
 * @brief Lightweight UART logging for the Cortex-M33.
 *
 * Uses UART4 at 115200 8N1 (blocking TX) so the M33 can print debug
 * messages to the board's USB debug serial port without needing a
 * semi-hosting or retargeting setup.
 *
 * Pin assignment (STM32MP257FDK — adjust AF if your board differs):
 *   UART4_TX : PD6  AF3
 *   UART4_RX : PD5  AF3
 */

#ifndef UART_LOG_H
#define UART_LOG_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise UART4 for logging. Call once from main() before first use. */
void uart_log_init(void);

/** Write a null-terminated string (no newline added). */
void uart_log_write(const char *str);

/** Write a null-terminated string followed by "\r\n". */
void uart_log_writeln(const char *str);

/** printf-style formatted output (internal buffer: 128 bytes). */
void uart_log_printf(const char *fmt, ...);

/** Print a 32-bit value as 8 hex digits, e.g. "0xDEADBEEF". */
void uart_log_hex32(uint32_t val);

/* ── Convenience macros ─────────────────────────────────────────────────── */
#define LOG_INFO(fmt, ...) uart_log_printf("[INFO]  " fmt "\r\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) uart_log_printf("[WARN]  " fmt "\r\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  uart_log_printf("[ERR]   " fmt "\r\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif /* UART_LOG_H */
