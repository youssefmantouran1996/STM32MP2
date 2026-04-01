/**
 * @file  ipc_handler.h
 * @brief Lightweight RPMsg/vring IPC handler for the Cortex-M33.
 *
 * Implements the minimal virtio vring protocol needed to communicate with
 * the Linux rpmsg_tty driver on the Cortex-A35, without the full OpenAMP
 * middleware stack.
 *
 * Usage:
 *   1. Call IPC_Init() once after remoteproc has started the M33.
 *      This reads vring addresses from the resource table, then sends the
 *      RPMsg Name-Service (NS) announcement so Linux creates /dev/ttyRPMSG0.
 *   2. Call IPC_Process() in the main loop to handle incoming messages
 *      and dispatch responses.
 *
 * Command protocol (A35 → M33, 5 bytes):
 *   [0xAA] [CMD] [ARG_LO] [ARG_HI] [0x55]
 *
 * Response protocol (M33 → A35, 8 bytes):
 *   [0xBB] [CMD] [ARG_LO] [ARG_HI] [STATUS] [TICK_B0] [TICK_B1] [0x55]
 *
 * Commands:
 *   IPC_CMD_PING        (0x01) — M33 replies with STATUS_OK + tick count
 *   IPC_CMD_LED_SET     (0x02) — ARG_LO: 0=off, 1=on
 *   IPC_CMD_LED_BLINK   (0x03) — ARG = blink interval in ms (uint16)
 *   IPC_CMD_GET_STATUS  (0x04) — M33 replies with current tick count
 *   IPC_CMD_GET_VERSION (0x05) — M33 replies: ARG_LO=major, ARG_HI=minor, STATUS=patch
 *   IPC_CMD_OTA_PREPARE (0x06) — M33 freezes IPC and replies ready for remoteproc stop
 *                                 STATUS: IPC_OTA_READY (0x01) or IPC_OTA_BUSY (0x02)
 */

#ifndef IPC_HANDLER_H
#define IPC_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Command identifiers ───────────────────────────────────────────────── */
#define IPC_CMD_PING         0x01U
#define IPC_CMD_LED_SET      0x02U
#define IPC_CMD_LED_BLINK    0x03U
#define IPC_CMD_GET_STATUS   0x04U
#define IPC_CMD_GET_VERSION  0x05U  /**< ARG_LO=major, ARG_HI=minor, STATUS=patch */
#define IPC_CMD_OTA_PREPARE  0x06U  /**< Graceful OTA prepare; M33 freezes IPC */

/* ── Status codes ──────────────────────────────────────────────────────── */
#define IPC_STATUS_OK        0x00U
#define IPC_STATUS_ERR       0xFFU
#define IPC_STATUS_UNKNOWN   0xFEU

/* ── OTA-specific status values (used in response to IPC_CMD_OTA_PREPARE) */
#define IPC_OTA_READY        0x01U  /**< M33 is ready for remoteproc stop */
#define IPC_OTA_BUSY         0x02U  /**< M33 cannot be updated right now   */

/* ── Frame markers ─────────────────────────────────────────────────────── */
#define IPC_SOF_CMD         0xAAU  /**< Start-of-frame: command (A35 → M33) */
#define IPC_SOF_RESP        0xBBU  /**< Start-of-frame: response (M33 → A35) */
#define IPC_EOF             0x55U  /**< End-of-frame marker */

/* ── RPMsg constants ───────────────────────────────────────────────────── */
#define RPMSG_MAX_PAYLOAD   496U   /**< Max payload bytes per message */
#define RPMSG_HDR_SIZE      16U    /**< rpmsg_hdr size in bytes */
#define RPMSG_BUF_SIZE      512U   /**< Total buffer size (hdr + payload) */
#define RPMSG_NS_ADDR       0x35U  /**< Well-known name-service endpoint */
#define RPMSG_ADDR_ANY      0xFFFFFFFFU

/* ── Command/response packet sizes ────────────────────────────────────── */
#define IPC_CMD_SIZE        5U     /**< [SOF][CMD][AL][AH][EOF] */
#define IPC_RESP_SIZE       8U     /**< [SOF][CMD][AL][AH][ST][T0][T1][EOF] */

/**
 * @brief Initialise the IPC subsystem.
 *
 * Reads vring addresses from the resource table (filled in by remoteproc),
 * sets up vring pointers, and sends the RPMsg Name-Service announcement
 * so that the Linux rpmsg_tty driver creates /dev/ttyRPMSG0.
 *
 * @note Must be called after remoteproc has started the M33 and populated
 *       the resource table vring addresses.
 */
void IPC_Init(void);

/**
 * @brief Process pending RPMsg messages (call in main loop).
 *
 * Checks the RX vring for new messages, dispatches commands, and sends
 * responses via the TX vring.  Non-blocking; returns immediately if no
 * message is available.
 */
void IPC_Process(void);

/**
 * @brief Send a raw payload to the A35 via the TX vring.
 *
 * @param data    Pointer to payload bytes.
 * @param length  Payload length (max RPMSG_MAX_PAYLOAD).
 * @return 0 on success, -1 if no TX buffer is available.
 */
int IPC_Send(const uint8_t *data, uint16_t length);

/**
 * @brief Returns non-zero after IPC_CMD_OTA_PREPARE has been received.
 *
 * The main loop should check this flag and halt normal operations so that
 * the A35 can safely stop the remoteproc and replace the firmware image.
 */
uint8_t IPC_IsOtaPending(void);

#ifdef __cplusplus
}
#endif
#endif /* IPC_HANDLER_H */
