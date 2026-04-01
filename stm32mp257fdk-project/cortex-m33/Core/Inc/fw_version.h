/**
 * @file  fw_version.h
 * @brief Firmware version constants for the Cortex-M33 image.
 *
 * The version triple is embedded in the ELF binary and reported to the A35
 * via IPC_CMD_GET_VERSION.  Bump the appropriate field on every release:
 *   MAJOR — breaking IPC protocol change
 *   MINOR — new feature, backwards-compatible
 *   PATCH — bug fix
 *
 * BUILD_TIMESTAMP is injected at compile time by CMake (-DFW_BUILD_TIMESTAMP=…).
 * If the flag is absent it defaults to 0.
 */

#ifndef FW_VERSION_H
#define FW_VERSION_H

#include <stdint.h>

#define FW_VERSION_MAJOR  1U
#define FW_VERSION_MINOR  0U
#define FW_VERSION_PATCH  0U

#ifndef FW_BUILD_TIMESTAMP
#define FW_BUILD_TIMESTAMP 0U
#endif

typedef struct {
    uint8_t  major;
    uint8_t  minor;
    uint8_t  patch;
    uint8_t  reserved;
    uint32_t build_timestamp; /**< Unix timestamp injected by CMake build */
} FwVersion;

/** Firmware version instance (defined in main.c). */
extern const FwVersion g_fw_version;

#endif /* FW_VERSION_H */
