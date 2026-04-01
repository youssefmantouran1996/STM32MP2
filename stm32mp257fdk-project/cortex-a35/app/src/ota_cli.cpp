/**
 * @file  ota_cli.cpp
 * @brief OTA update command-line tool — Cortex-A35 Linux userspace.
 *
 * Opens the RPMsg channel to the M33, optionally queries the running version,
 * then performs a full OTA update via OtaManager.
 *
 * Usage:
 *   ota_update <firmware.elf> [firmware.sha256] [/dev/ttyRPMSG0]
 *
 * Arguments:
 *   firmware.elf     New M33 ELF image to install (required)
 *   firmware.sha256  SHA256 checksum file for the ELF (optional; skip with "")
 *   /dev/ttyRPMSGx   RPMsg device (default: /dev/ttyRPMSG0)
 *
 * Exit codes:
 *   0  — OTA succeeded (Result::Ok)
 *   1  — OTA failed or argument error
 */

#include "ipc_rpmsg.h"
#include "ota_manager.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <firmware.elf> [firmware.sha256] [/dev/ttyRPMSG0]\n";
        return EXIT_FAILURE;
    }

    const std::string fw_path  = argv[1];
    const std::string sha_path = (argc > 2) ? argv[2] : "";

    ota::Config cfg;
    if (argc > 3) {
        cfg.rpmsg_device = argv[3];
    }

    ipc::RpmsgChannel channel(cfg.rpmsg_device, cfg.prepare_timeout_ms);
    if (!channel.open()) {
        std::cerr << "[OTA] Cannot open RPMsg channel: " << cfg.rpmsg_device << "\n"
                  << "      Is the M33 firmware running?\n"
                  << "        echo start > /sys/class/remoteproc/remoteproc0/state\n";
        return EXIT_FAILURE;
    }

    ota::OtaManager mgr(cfg);

    /* Optional: show current running version */
    const ota::Version ver = mgr.get_version(channel);
    if (ver.major != 0U || ver.minor != 0U || ver.patch != 0U) {
        std::cout << "[OTA] Running M33 firmware: v"
                  << static_cast<int>(ver.major) << "."
                  << static_cast<int>(ver.minor) << "."
                  << static_cast<int>(ver.patch) << "\n";
    }

    std::cout << "[OTA] Firmware image : " << fw_path  << "\n";
    if (!sha_path.empty()) {
        std::cout << "[OTA] SHA256 file    : " << sha_path << "\n";
    }
    std::cout << "[OTA] RPMsg device   : " << cfg.rpmsg_device << "\n\n";

    const ota::Result result = mgr.update(fw_path, sha_path, channel);

    std::cout << "\n[OTA] Result: " << ota::result_str(result) << "\n";
    return (result == ota::Result::Ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
