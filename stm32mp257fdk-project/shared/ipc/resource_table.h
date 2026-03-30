/**
 * @file  resource_table.h
 * @brief OpenAMP resource table — shared between Cortex-A35 and Cortex-M33.
 *
 * The resource table tells remoteproc on the A35 where to find:
 *   - The vring descriptors (shared ring buffers for OpenAMP)
 *   - The shared memory region
 *
 * remoteproc on the A35 scans the M33 firmware ELF for an ELF section
 * named ".resource_table", reads the version/count, then fills in the
 * allocated DDR addresses before starting the M33 core.
 *
 * Both cores must agree on this layout.  The actual instance lives in
 * shared/ipc/resource_table.c and is linked into .resource_table.
 *
 * Reference: Linux kernel include/linux/remoteproc.h, ST AN5903.
 */

#ifndef RESOURCE_TABLE_H
#define RESOURCE_TABLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Resource-entry type IDs (from Linux remoteproc.h) ─────────────────── */
#define RSC_VDEV            3U   /**< Virtio device */

/* ── Virtio device / RPMsg identifiers ─────────────────────────────────── */
#define VIRTIO_ID_RPMSG     7U   /**< Virtio RPMsg device ID */
#define VIRTIO_RPMSG_F_NS   (1U << 0)  /**< Name-service feature bit */

/* ── Vring configuration ─────────────────────────────────────────────────
 * 16 buffers × 512-byte each = 8 KB per vring buffer pool.
 * 0xFFFFFFFF tells remoteproc to allocate the address dynamically.
 */
#define VRING_NUM_BUFFERS   16U
#define VRING_ALIGNMENT     16U
#define VRING_DA_AUTO       0xFFFFFFFFU  /**< Let remoteproc allocate */

/* ── Shared DDR region ──────────────────────────────────────────────────
 * Must match the carve-out reservation in the Linux device tree.
 */
#define SHM_BASE_ADDR       0x87E00000UL
#define SHM_SIZE            0x00100000UL  /* 1 MB */

/* ── vring descriptor (virtio spec) ─────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint64_t addr;   /**< Physical address of the buffer */
    uint32_t len;    /**< Length of the buffer */
    uint16_t flags;  /**< VRING_DESC_F_* */
    uint16_t next;   /**< Chaining index when VRING_DESC_F_NEXT is set */
} VringDesc;

#define VRING_DESC_F_NEXT   1U
#define VRING_DESC_F_WRITE  2U

/* ── Available ring ──────────────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VRING_NUM_BUFFERS];
    uint16_t used_event;
} VringAvail;

/* ── Used ring ───────────────────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t id;
    uint32_t len;
} VringUsedElem;

typedef struct __attribute__((packed)) {
    uint16_t     flags;
    uint16_t     idx;
    VringUsedElem ring[VRING_NUM_BUFFERS];
    uint16_t     avail_event;
} VringUsed;

/* ── fw_rsc_vdev_vring (matches Linux kernel struct) ────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t da;        /**< Device address (0xFFFFFFFF = auto-alloc) */
    uint32_t align;     /**< Alignment requirement */
    uint32_t num;       /**< Number of vring descriptors */
    uint32_t notifyid;  /**< IPCC channel / notification ID */
    uint32_t pa;        /**< Physical address (filled by remoteproc) */
} FwRscVdevVring;

/* ── fw_rsc_vdev (matches Linux kernel struct) ───────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t      type;           /**< RSC_VDEV */
    uint32_t      id;             /**< VIRTIO_ID_RPMSG */
    uint32_t      notifyid;       /**< Notification ID */
    uint32_t      dfeatures;      /**< Device features offered */
    uint32_t      gfeatures;      /**< Guest features accepted */
    uint32_t      config_len;     /**< Size of config space (0 for rpmsg) */
    uint8_t       status;         /**< Driver status byte */
    uint8_t       num_of_vrings;  /**< Number of vrings (2: TX + RX) */
    uint8_t       reserved[2];
    FwRscVdevVring vring[2];      /**< vring[0]=M33 TX, vring[1]=M33 RX */
} FwRscVdev;

/* ── Full resource table ─────────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    /* Table header */
    uint32_t  ver;          /**< Must be 1 */
    uint32_t  num;          /**< Number of resource entries */
    uint32_t  reserved[2];
    uint32_t  offset[1];    /**< Byte offsets from table start to each entry */
    /* Single entry: VDEV */
    FwRscVdev vdev;
} ResourceTable;

/** Extern declaration — instance defined in shared/ipc/resource_table.c */
extern ResourceTable resource_table;

#ifdef __cplusplus
}
#endif
#endif /* RESOURCE_TABLE_H */
