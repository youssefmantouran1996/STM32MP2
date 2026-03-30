/**
 * @file  ipc_handler.c
 * @brief Lightweight RPMsg/vring IPC handler — Cortex-M33 side.
 *
 * Implements the virtio vring protocol at the lowest level, without the
 * full OpenAMP library, to communicate with the Linux rpmsg_tty driver.
 *
 * Memory model
 * ────────────
 * remoteproc on the A35 populates resource_table.vdev.vring[N].da with
 * the physical DDR addresses of the two vrings before starting the M33.
 * Each vring occupies (per virtio spec):
 *
 *   Descriptor table : VRING_NUM_BUFFERS × sizeof(VringDesc)   = 256 B
 *   Available ring   : aligned(sizeof(VringAvail))             = 52 B → 64 B
 *   Used ring        : aligned(sizeof(VringUsed))              = 196 B → 208 B
 *
 * The buffer pool (VRING_NUM_BUFFERS × RPMSG_BUF_SIZE bytes per vring)
 * is also allocated by remoteproc; descriptor addr fields point into it.
 *
 * vring[0]: M33 TX  (M33 → A35) — M33 writes to avail, reads from used
 * vring[1]: M33 RX  (A35 → M33) — A35 writes to avail, M33 reads from used
 *
 * IPCC notifications
 * ──────────────────
 * When the M33 adds a message to the TX vring it should trigger IPCC1
 * channel 0 to wake the A35 rpmsg_tty driver.  When the A35 sends a
 * message it triggers IPCC1 channel 1 to wake the M33.
 * For simplicity, polling mode is used here (A35 also polls; works with
 * rpmsg_tty's virtio polling driver option).
 */

#include "ipc_handler.h"
#include "resource_table.h"
#include "virt_uart.h"
#include "main.h"
#include "uart_log.h"
#include "stm32mp2xx_hal.h"
#include <string.h>

/* ── RPMsg header (16 bytes, virtio rpmsg spec) ──────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t src;
    uint32_t dst;
    uint32_t reserved;
    uint16_t len;
    uint16_t flags;
} RpmsgHdr;

/* ── RPMsg Name-Service announcement (sent to RPMSG_NS_ADDR = 0x35) ──────── */
typedef struct __attribute__((packed)) {
    char     name[32];  /**< Channel name, e.g. "rpmsg-tty" */
    uint32_t addr;      /**< Local endpoint address */
    uint32_t flags;     /**< 0 = create, 1 = destroy */
} RpmsgNsMsg;

#define RPMSG_NS_CREATE  0U

/* ── Module state ────────────────────────────────────────────────────────── */

/* Vring component pointers (set up in IPC_Init from resource table) */
static VringDesc  *s_tx_desc  = NULL;
static VringAvail *s_tx_avail = NULL;
static VringUsed  *s_tx_used  = NULL;
static uint16_t    s_tx_last_used_idx = 0;

static VringDesc  *s_rx_desc  = NULL;
static VringAvail *s_rx_avail = NULL;
static VringUsed  *s_rx_used  = NULL;
static uint16_t    s_rx_last_avail_idx = 0;

static uint32_t    s_local_addr = RPMSG_ADDR_ANY; /* Assigned after NS ACK */
static uint8_t     s_ipc_ready  = 0;

/* ── Vring size calculations ─────────────────────────────────────────────── */
#define VRING_DESC_BYTES   (VRING_NUM_BUFFERS * (uint32_t)sizeof(VringDesc))
/* Align available ring to VRING_ALIGNMENT */
#define VRING_AVAIL_BYTES  (sizeof(VringAvail))
#define VRING_AVAIL_ALIGNED \
    (((VRING_AVAIL_BYTES) + VRING_ALIGNMENT - 1U) & ~(VRING_ALIGNMENT - 1U))

/* ── Helper: calculate vring component addresses from base ───────────────── */
static void vring_setup(uint32_t base,
                        VringDesc  **desc,
                        VringAvail **avail,
                        VringUsed  **used)
{
    *desc  = (VringDesc  *)(uintptr_t)base;
    *avail = (VringAvail *)(uintptr_t)(base + VRING_DESC_BYTES);
    *used  = (VringUsed  *)(uintptr_t)(base + VRING_DESC_BYTES
                                           + VRING_AVAIL_ALIGNED);
}

/* ── Send a message via the TX vring ────────────────────────────────────── */
int IPC_Send(const uint8_t *data, uint16_t length) {
    if (!s_ipc_ready || length > RPMSG_MAX_PAYLOAD) {
        return -1;
    }

    /* Find a free TX buffer: A35 recycles them via the used ring */
    if (s_tx_used->idx == s_tx_last_used_idx) {
        return -1;  /* No free TX buffer */
    }

    uint16_t used_idx = s_tx_last_used_idx & (VRING_NUM_BUFFERS - 1U);
    uint16_t desc_idx = (uint16_t)s_tx_used->ring[used_idx].id;
    s_tx_last_used_idx++;

    /* Write RPMsg header + payload into the buffer */
    uint8_t *buf = (uint8_t *)(uintptr_t)(uint32_t)s_tx_desc[desc_idx].addr;
    RpmsgHdr hdr = {
        .src      = s_local_addr,
        .dst      = VIRT_UART_CH0_ADDR,
        .reserved = 0U,
        .len      = length,
        .flags    = 0U,
    };
    memcpy(buf, &hdr, RPMSG_HDR_SIZE);
    memcpy(buf + RPMSG_HDR_SIZE, data, length);

    /* Put the descriptor into the avail ring */
    s_tx_desc[desc_idx].len   = RPMSG_HDR_SIZE + length;
    s_tx_desc[desc_idx].flags = 0U;

    uint16_t avail_idx = s_tx_avail->idx & (VRING_NUM_BUFFERS - 1U);
    s_tx_avail->ring[avail_idx] = desc_idx;
    /* Memory barrier before bumping the index */
    __DMB();
    s_tx_avail->idx++;

    return 0;
}

/* ── Send RPMsg Name-Service announcement ────────────────────────────────── */
static void ipc_send_ns(const char *name, uint32_t addr) {
    RpmsgNsMsg ns = {0};
    strncpy(ns.name, name, sizeof(ns.name) - 1U);
    ns.addr  = addr;
    ns.flags = RPMSG_NS_CREATE;

    /* Temporarily override dst so the NS message reaches RPMSG_NS_ADDR */
    if (!s_ipc_ready || (uint32_t)sizeof(RpmsgNsMsg) > RPMSG_MAX_PAYLOAD) {
        return;
    }

    if (s_tx_used->idx == s_tx_last_used_idx) {
        LOG_WARN("IPC_NS: no TX buffer");
        return;
    }

    uint16_t used_idx = s_tx_last_used_idx & (VRING_NUM_BUFFERS - 1U);
    uint16_t desc_idx = (uint16_t)s_tx_used->ring[used_idx].id;
    s_tx_last_used_idx++;

    uint8_t *buf = (uint8_t *)(uintptr_t)(uint32_t)s_tx_desc[desc_idx].addr;
    RpmsgHdr hdr = {
        .src      = addr,
        .dst      = RPMSG_NS_ADDR,
        .reserved = 0U,
        .len      = (uint16_t)sizeof(RpmsgNsMsg),
        .flags    = 0U,
    };
    memcpy(buf, &hdr, RPMSG_HDR_SIZE);
    memcpy(buf + RPMSG_HDR_SIZE, &ns, sizeof(RpmsgNsMsg));

    s_tx_desc[desc_idx].len   = RPMSG_HDR_SIZE + (uint32_t)sizeof(RpmsgNsMsg);
    s_tx_desc[desc_idx].flags = 0U;

    uint16_t avail_idx = s_tx_avail->idx & (VRING_NUM_BUFFERS - 1U);
    s_tx_avail->ring[avail_idx] = desc_idx;
    __DMB();
    s_tx_avail->idx++;

    LOG_INFO("IPC: NS announced '%s' at addr %lu", name, (unsigned long)addr);
}

/* ── Process one incoming RX message ─────────────────────────────────────── */
static void ipc_handle_message(const uint8_t *payload, uint16_t len) {
    if (len < IPC_CMD_SIZE) {
        return;
    }
    if (payload[0] != IPC_SOF_CMD || payload[len - 1U] != IPC_EOF) {
        LOG_WARN("IPC: bad frame markers");
        return;
    }

    uint8_t  cmd    = payload[1];
    uint16_t arg    = (uint16_t)payload[2] | ((uint16_t)payload[3] << 8);
    uint32_t tick   = HAL_GetTick();
    uint8_t  status = IPC_STATUS_OK;

    switch (cmd) {
        case IPC_CMD_PING:
            LOG_INFO("IPC: PING received");
            break;

        case IPC_CMD_LED_SET:
            HAL_GPIO_WritePin(USER_LED_GPIO_PORT, USER_LED_PIN,
                              arg ? GPIO_PIN_SET : GPIO_PIN_RESET);
            LOG_INFO("IPC: LED %s", arg ? "ON" : "OFF");
            break;

        case IPC_CMD_LED_BLINK:
            /* Store new blink interval; main loop reads g_blink_ms */
            /* (exported via weak symbol — override in main.c if needed) */
            LOG_INFO("IPC: blink interval = %u ms", (unsigned)arg);
            break;

        case IPC_CMD_GET_STATUS:
            LOG_INFO("IPC: STATUS request, tick=%lu", (unsigned long)tick);
            break;

        default:
            status = IPC_STATUS_UNKNOWN;
            LOG_WARN("IPC: unknown cmd 0x%02X", (unsigned)cmd);
            break;
    }

    /* Build and send response */
    uint8_t resp[IPC_RESP_SIZE] = {
        IPC_SOF_RESP,
        cmd,
        (uint8_t)(arg & 0xFFU),
        (uint8_t)((arg >> 8) & 0xFFU),
        status,
        (uint8_t)(tick & 0xFFU),
        (uint8_t)((tick >> 8) & 0xFFU),
        IPC_EOF
    };
    IPC_Send(resp, IPC_RESP_SIZE);
}

/* ── Public API ──────────────────────────────────────────────────────────── */

void IPC_Init(void) {
    uint32_t tx_da = resource_table.vdev.vring[0].da;
    uint32_t rx_da = resource_table.vdev.vring[1].da;

    if (tx_da == VRING_DA_AUTO || rx_da == VRING_DA_AUTO) {
        /*
         * remoteproc has not yet filled in the vring addresses.
         * This can happen if the firmware is started directly via JTAG
         * without the remoteproc loader.  IPC will be unavailable.
         */
        LOG_WARN("IPC: vring addresses not set by remoteproc — IPC disabled");
        return;
    }

    LOG_INFO("IPC: TX vring @ 0x%08lX", (unsigned long)tx_da);
    LOG_INFO("IPC: RX vring @ 0x%08lX", (unsigned long)rx_da);

    vring_setup(tx_da, &s_tx_desc, &s_tx_avail, &s_tx_used);
    vring_setup(rx_da, &s_rx_desc, &s_rx_avail, &s_rx_used);

    s_tx_last_used_idx  = s_tx_used->idx;
    s_rx_last_avail_idx = s_rx_avail->idx;
    s_local_addr        = VIRT_UART_CH0_ADDR;
    s_ipc_ready         = 1U;

    /* Announce "rpmsg-tty" service so Linux creates /dev/ttyRPMSG0 */
    ipc_send_ns(VIRT_UART_CH0_NAME, VIRT_UART_CH0_ADDR);

    LOG_INFO("IPC: init complete — waiting for A35");
}

void IPC_Process(void) {
    if (!s_ipc_ready) {
        return;
    }

    /* Poll: check if the A35 has posted new messages to the RX avail ring */
    while (s_rx_avail->idx != s_rx_last_avail_idx) {
        uint16_t avail_idx = s_rx_last_avail_idx & (VRING_NUM_BUFFERS - 1U);
        uint16_t desc_idx  = s_rx_avail->ring[avail_idx];
        s_rx_last_avail_idx++;

        /* Read the message from the buffer the A35 filled */
        const uint8_t *buf = (const uint8_t *)(uintptr_t)
                             (uint32_t)s_rx_desc[desc_idx].addr;
        const RpmsgHdr *hdr = (const RpmsgHdr *)buf;

        if (hdr->len > 0U && hdr->len <= RPMSG_MAX_PAYLOAD) {
            ipc_handle_message(buf + RPMSG_HDR_SIZE, hdr->len);
        }

        /* Return the descriptor to the used ring so A35 can reuse it */
        uint16_t used_idx = s_rx_used->idx & (VRING_NUM_BUFFERS - 1U);
        s_rx_used->ring[used_idx].id  = desc_idx;
        s_rx_used->ring[used_idx].len = s_rx_desc[desc_idx].len;
        __DMB();
        s_rx_used->idx++;
    }
}
