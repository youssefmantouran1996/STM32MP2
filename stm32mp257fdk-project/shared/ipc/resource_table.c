/**
 * @file  resource_table.c
 * @brief OpenAMP resource table instance — placed in .resource_table ELF section.
 *
 * remoteproc on the A35 searches the firmware ELF for a section named
 * ".resource_table", parses the version/count header, then:
 *   1. Allocates DDR for each vring and fills in the `da` and `pa` fields.
 *   2. Kicks the M33 core to start execution.
 *
 * The M33 then reads resource_table.vdev.vring[N].da to find the vring
 * base addresses at runtime.
 *
 * Compiled for both M33 (cortex-m33/CMakeLists.txt) and A35 if needed.
 */

#include "resource_table.h"
#include <stddef.h>

/*
 * __attribute__((used)) prevents the linker from discarding the symbol
 * even though no C code directly references it — remoteproc finds it
 * by ELF section name, not by symbol name.
 */
ResourceTable __attribute__((section(".resource_table")))
              __attribute__((used))
resource_table = {
    /* ── Header ─────────────────────────────────────────────────────────── */
    .ver      = 1U,
    .num      = 1U,
    .reserved = {0U, 0U},

    /*
     * offset[0] = byte distance from the start of ResourceTable to .vdev.
     * This is the offset of the FwRscVdev field within the struct.
     */
    .offset = {
        (uint32_t)offsetof(ResourceTable, vdev)
    },

    /* ── VDEV entry (RPMsg virtio device) ────────────────────────────────── */
    .vdev = {
        .type          = RSC_VDEV,
        .id            = VIRTIO_ID_RPMSG,
        .notifyid      = 0U,
        .dfeatures     = VIRTIO_RPMSG_F_NS,  /* Advertise Name-Service */
        .gfeatures     = 0U,
        .config_len    = 0U,
        .status        = 0U,
        .num_of_vrings = 2U,
        .reserved      = {0U, 0U},

        .vring = {
            /* vring[0]: M33 TX  (M33 → A35) — notifyid must be unique */
            {
                .da       = VRING_DA_AUTO,
                .align    = VRING_ALIGNMENT,
                .num      = VRING_NUM_BUFFERS,
                .notifyid = 0U,
                .pa       = 0U,
            },
            /* vring[1]: M33 RX  (A35 → M33) */
            {
                .da       = VRING_DA_AUTO,
                .align    = VRING_ALIGNMENT,
                .num      = VRING_NUM_BUFFERS,
                .notifyid = 1U,
                .pa       = 0U,
            },
        },
    },
};
