/**
 * @file  virt_uart.h
 * @brief Virtual UART channel IDs shared between A35 and M33.
 *
 * The virtual UART is implemented on top of RPMsg. Each channel ID
 * corresponds to a /dev/ttyRPMSGx device on the Linux A35 side and
 * a VIRT_UART handle on the M33 side (STM32CubeMP2 VIRT_UART middleware).
 */

#ifndef VIRT_UART_H
#define VIRT_UART_H

/* Channel 0: General-purpose command / response */
#define VIRT_UART_CH0_NAME  "rpmsg-tty"
#define VIRT_UART_CH0_ADDR  0U

/* Channel 1: Telemetry / sensor data stream */
#define VIRT_UART_CH1_NAME  "rpmsg-tty"
#define VIRT_UART_CH1_ADDR  1U

/* Maximum RPMsg payload size (bytes) */
#define RPMSG_MAX_PAYLOAD   496U

#endif /* VIRT_UART_H */
