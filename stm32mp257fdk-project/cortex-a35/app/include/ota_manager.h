/**
 * @file  ota_manager.h
 * @brief Over-The-Air firmware update manager for the Cortex-M33 coprocessor.
 *
 * Update flow
 * ───────────
 * 1. Query M33 version via IPC_CMD_GET_VERSION (optional, for version check)
 * 2. Send IPC_CMD_OTA_PREPARE; M33 freezes IPC and ACKs
 * 3. Stop M33 via remoteproc sysfs:
 *      echo stop > /sys/class/remoteproc/remoteproc0/state
 * 4. Verify new firmware SHA256 (if a checksum file is provided)
 * 5. Back up current firmware, install new .elf to /lib/firmware/
 * 6. Start M33 via remoteproc:
 *      echo start > /sys/class/remoteproc/remoteproc0/state
 * 7. Wait for /dev/ttyRPMSG0 to reappear, ping M33 to confirm health
 * 8. On any failure: roll back to the backup and restart — report result
 *
 * The remoteproc sysfs path and firmware name are configurable via OtaConfig
 * to accommodate different board bringup environments.
 */

#pragma once

#include "ipc_rpmsg.h"
#include <cstdint>
#include <string>

namespace ota {

/* ── Configuration ──────────────────────────────────────────────────────── */
struct Config {
    /** Path to the remoteproc sysfs directory for the M33. */
    std::string remoteproc_path  = "/sys/class/remoteproc/remoteproc0";

    /** Directory where remoteproc looks for firmware images. */
    std::string firmware_dir     = "/lib/firmware";

    /** Name of the firmware file inside firmware_dir. */
    std::string firmware_name    = "stm32mp257fdk_m33.elf";

    /** Path to the RPMsg virtual UART device (recreated after M33 restart). */
    std::string rpmsg_device     = "/dev/ttyRPMSG0";

    /** Milliseconds to wait for M33 to ACK OTA_PREPARE. */
    int prepare_timeout_ms       = 3000;

    /** Milliseconds to wait for remoteproc state transition (stop/start). */
    int remoteproc_timeout_ms    = 10000;

    /** Milliseconds to wait for /dev/ttyRPMSG0 after M33 restart. */
    int rpmsg_appear_timeout_ms  = 15000;

    /** Milliseconds to wait for M33 PING response after restart. */
    int health_ping_timeout_ms   = 5000;
};

/* ── Firmware version ────────────────────────────────────────────────────── */
struct Version {
    uint8_t major{0};
    uint8_t minor{0};
    uint8_t patch{0};

    bool operator==(const Version &o) const {
        return major == o.major && minor == o.minor && patch == o.patch;
    }
    bool operator!=(const Version &o) const { return !(*this == o); }
};

/* ── Result codes ────────────────────────────────────────────────────────── */
enum class Result {
    Ok,               /**< Update succeeded */
    PrepareTimeout,   /**< M33 did not ACK OTA_PREPARE in time */
    StopFailed,       /**< remoteproc stop failed or timed out */
    VerifyFailed,     /**< SHA256 mismatch on new firmware */
    InstallFailed,    /**< Could not copy new firmware to /lib/firmware/ */
    StartFailed,      /**< remoteproc start failed or timed out */
    HealthCheckFailed,/**< M33 did not respond to PING after restart */
    RollbackOk,       /**< Update failed; rollback to previous version succeeded */
    RollbackFailed,   /**< Update and rollback both failed */
};

const char *result_str(Result r);

/* ── OtaManager ─────────────────────────────────────────────────────────── */
class OtaManager {
public:
    explicit OtaManager(Config cfg = {});

    /**
     * Query the running M33 firmware version.
     * Returns Version{0,0,0} if the M33 is unreachable.
     */
    Version get_version(ipc::RpmsgChannel &channel);

    /**
     * Perform a full OTA update.
     *
     * @param new_fw_path   Path to the new .elf firmware file on the A35
     *                      filesystem.
     * @param sha256_path   Path to a SHA256 checksum file (one line:
     *                      "<hex> <filename>").  Pass "" to skip verification.
     * @param channel       Open RPMsg channel used to signal the M33 before
     *                      the update and to verify health afterwards.
     *                      The channel is closed and reopened internally.
     */
    Result update(const std::string &new_fw_path,
                  const std::string &sha256_path,
                  ipc::RpmsgChannel &channel);

private:
    Config cfg_;

    bool write_state(const std::string &state);
    std::string read_state();
    bool wait_for_state(const std::string &expected, int timeout_ms);
    bool wait_for_device(const std::string &path, int timeout_ms);
    bool prepare_m33(ipc::RpmsgChannel &channel);
    bool verify_sha256(const std::string &file_path,
                       const std::string &sha256_path);
    bool install_firmware(const std::string &src,
                          const std::string &backup_out);
    bool restore_firmware(const std::string &backup_path);
    bool health_check(ipc::RpmsgChannel &channel);
};

}  // namespace ota
