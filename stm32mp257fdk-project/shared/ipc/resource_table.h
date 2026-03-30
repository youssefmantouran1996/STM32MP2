/**
 * @file  resource_table.h
 * @brief OpenAMP resource table — shared between Cortex-A35 and Cortex-M33.
 *
 * The resource table tells the A35 (remoteproc / rpmsg) where to find:
 *   - The vring descriptors (shared ring buffers used by OpenAMP)
 *   - Any other resources the M33 firmware needs (trace buffer, etc.)
 *
 * Both cores must agree on the layout of this table and the base address.
 * Typically placed in a non-cached shared DDR region.
 *
 * Reference: OpenAMP specification, ST AN5903.
 */

#ifndef RESOURCE_TABLE_H
#define RESOURCE_TABLE_H

#include <stdint.h>

/* ── Vring configuration ─────────────────────────────────────────────────── */
#define VRING_COUNT         2U     /* Number of vrings (TX + RX) */
#define VRING_ALIGNMENT     16U    /* Descriptor alignment (bytes) */
#define VRING_NUM_BUFFERS   16U    /* Number of buffers per vring */
#define VRING_RX_ADDR       0xFFFFFFFFU  /* Let remoteproc allocate */
#define VRING_TX_ADDR       0xFFFFFFFFU

/* ── Resource table offsets ──────────────────────────────────────────────── */
#define RSC_TABLE_NUM_ENTRIES   1U
#define VDEV_FEATURES           0U

/* ── Shared memory layout ────────────────────────────────────────────────── */
/* Adjust base address to match your DDR reservation in the device tree.     */
#define SHM_BASE_ADDR       0x87E00000UL
#define SHM_SIZE            0x00100000UL  /* 1 MB */

/* ── OpenAMP resource table structure ───────────────────────────────────────
 *
 * Example structure — use the definitions from the STM32CubeMP2 OpenAMP
 * middleware (openamp/lib/include/openamp/remoteproc.h) for the actual types.
 *
 * typedef struct {
 *     uint32_t ver;
 *     uint32_t num;
 *     uint32_t reserved[2];
 *     uint32_t offset[RSC_TABLE_NUM_ENTRIES];
 *     struct fw_rsc_vdev vdev;
 *     struct fw_rsc_vdev_vring vring0;
 *     struct fw_rsc_vdev_vring vring1;
 * } ResourceTable;
 *
 * extern ResourceTable resource_table;
 */

#endif /* RESOURCE_TABLE_H */
