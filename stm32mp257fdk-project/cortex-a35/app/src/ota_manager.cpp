/**
 * @file  ota_manager.cpp
 * @brief OTA firmware update manager — Cortex-A35 Linux userspace.
 *
 * Uses the Linux remoteproc sysfs interface to stop the M33, replace the
 * firmware image in /lib/firmware/, and restart the M33.  The M33 is
 * signalled via IPC_CMD_OTA_PREPARE before the stop so it can freeze its
 * vring access.
 *
 * Filesystem layout assumed on the target:
 *   /lib/firmware/stm32mp257fdk_m33.elf          — active firmware
 *   /lib/firmware/stm32mp257fdk_m33.elf.bak      — backup created during OTA
 *   /sys/class/remoteproc/remoteproc0/state      — "running" / "offline"
 *   /sys/class/remoteproc/remoteproc0/firmware   — current firmware name
 */

#include "ota_manager.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace ota {

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static bool sysfs_write(const std::string &path, const std::string &value) {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[OTA] Cannot write '" << value << "' to " << path << "\n";
        return false;
    }
    f << value;
    return f.good();
}

static std::string sysfs_read(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::string s;
    std::getline(f, s);
    return s;
}

static bool file_exists(const std::string &path) {
    return access(path.c_str(), F_OK) == 0;
}

/** Copy src → dst using POSIX read/write in 64 KB chunks. */
static bool copy_file(const std::string &src, const std::string &dst) {
    int in = ::open(src.c_str(), O_RDONLY);
    if (in < 0) {
        std::cerr << "[OTA] Cannot open source: " << src << "\n";
        return false;
    }
    int out = ::open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        ::close(in);
        std::cerr << "[OTA] Cannot open dest: " << dst << "\n";
        return false;
    }

    std::array<char, 65536> buf{};
    ssize_t n;
    bool ok = true;
    while ((n = ::read(in, buf.data(), buf.size())) > 0) {
        if (::write(out, buf.data(), static_cast<size_t>(n)) != n) {
            ok = false;
            break;
        }
    }
    if (n < 0) ok = false;

    ::close(in);
    ::close(out);
    return ok;
}

const char *result_str(Result r) {
    switch (r) {
        case Result::Ok:               return "Ok";
        case Result::PrepareTimeout:   return "PrepareTimeout";
        case Result::StopFailed:       return "StopFailed";
        case Result::VerifyFailed:     return "VerifyFailed";
        case Result::InstallFailed:    return "InstallFailed";
        case Result::StartFailed:      return "StartFailed";
        case Result::HealthCheckFailed:return "HealthCheckFailed";
        case Result::RollbackOk:       return "RollbackOk";
        case Result::RollbackFailed:   return "RollbackFailed";
        default:                       return "Unknown";
    }
}

/* ── OtaManager ─────────────────────────────────────────────────────────── */

OtaManager::OtaManager(Config cfg) : cfg_(std::move(cfg)) {}

/* ── Private helpers ────────────────────────────────────────────────────── */

bool OtaManager::write_state(const std::string &state) {
    return sysfs_write(cfg_.remoteproc_path + "/state", state);
}

std::string OtaManager::read_state() {
    return sysfs_read(cfg_.remoteproc_path + "/state");
}

bool OtaManager::wait_for_state(const std::string &expected, int timeout_ms) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(timeout_ms);
    while (steady_clock::now() < deadline) {
        if (read_state() == expected) return true;
        std::this_thread::sleep_for(milliseconds(200));
    }
    std::cerr << "[OTA] Timed out waiting for remoteproc state '" << expected
              << "' (got '" << read_state() << "')\n";
    return false;
}

bool OtaManager::wait_for_device(const std::string &path, int timeout_ms) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(timeout_ms);
    while (steady_clock::now() < deadline) {
        if (file_exists(path)) return true;
        std::this_thread::sleep_for(milliseconds(500));
    }
    std::cerr << "[OTA] Timed out waiting for device " << path << "\n";
    return false;
}

bool OtaManager::prepare_m33(ipc::RpmsgChannel &channel) {
    using namespace std::chrono;

    std::cout << "[OTA] Sending OTA_PREPARE to M33...\n";
    ssize_t sent = channel.send_cmd(ipc::Cmd::OtaPrepare, 0);
    if (sent <= 0) {
        std::cerr << "[OTA] Failed to send OTA_PREPARE\n";
        return false;
    }

    /* Re-use RpmsgChannel::receive_resp with the configured timeout */
    ipc::RespPacket resp{};
    if (!channel.receive_resp(resp)) {
        std::cerr << "[OTA] No response to OTA_PREPARE (M33 may already be offline)\n";
        return false;
    }

    if (resp.cmd != static_cast<uint8_t>(ipc::Cmd::OtaPrepare)) {
        std::cerr << "[OTA] Unexpected cmd in OTA_PREPARE response: 0x"
                  << std::hex << static_cast<int>(resp.cmd) << "\n";
        return false;
    }

    /* status == 0x01 means IPC_OTA_READY */
    if (resp.status != 0x01U) {
        std::cerr << "[OTA] M33 responded BUSY (status=0x" << std::hex
                  << static_cast<int>(resp.status) << ") — cannot update now\n";
        return false;
    }

    std::cout << "[OTA] M33 acknowledged OTA_PREPARE — IPC frozen\n";
    return true;
}

bool OtaManager::verify_sha256(const std::string &file_path,
                                const std::string &sha256_path) {
    if (sha256_path.empty()) return true;  /* Verification skipped */

    /* Read expected hash from the .sha256 file (format: "<hex>  <name>") */
    std::ifstream hf(sha256_path);
    if (!hf.is_open()) {
        std::cerr << "[OTA] Cannot open SHA256 file: " << sha256_path << "\n";
        return false;
    }
    std::string expected_hash;
    hf >> expected_hash;
    if (expected_hash.size() != 64U) {
        std::cerr << "[OTA] Invalid SHA256 file format\n";
        return false;
    }

    /* Compute hash via sha256sum */
    std::string cmd = "sha256sum \"" + file_path + "\"";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "[OTA] sha256sum not available\n";
        return false;
    }
    std::array<char, 256> buf{};
    std::string output;
    while (fgets(buf.data(), buf.size(), pipe)) {
        output += buf.data();
    }
    pclose(pipe);

    std::string actual_hash = output.substr(0, 64);
    if (actual_hash != expected_hash) {
        std::cerr << "[OTA] SHA256 mismatch!\n"
                  << "  expected: " << expected_hash << "\n"
                  << "  actual  : " << actual_hash   << "\n";
        return false;
    }

    std::cout << "[OTA] SHA256 verified OK\n";
    return true;
}

bool OtaManager::install_firmware(const std::string &src,
                                   const std::string &backup_out) {
    std::string active = cfg_.firmware_dir + "/" + cfg_.firmware_name;

    /* Back up the current firmware */
    if (file_exists(active)) {
        std::cout << "[OTA] Backing up " << active << " → " << backup_out << "\n";
        if (!copy_file(active, backup_out)) {
            std::cerr << "[OTA] Backup failed\n";
            return false;
        }
    }

    std::cout << "[OTA] Installing " << src << " → " << active << "\n";
    if (!copy_file(src, active)) {
        std::cerr << "[OTA] Install failed\n";
        return false;
    }

    std::cout << "[OTA] Firmware installed\n";
    return true;
}

bool OtaManager::restore_firmware(const std::string &backup_path) {
    if (!file_exists(backup_path)) {
        std::cerr << "[OTA] No backup found at " << backup_path << "\n";
        return false;
    }
    std::string active = cfg_.firmware_dir + "/" + cfg_.firmware_name;
    std::cout << "[OTA] Restoring backup " << backup_path << " → " << active << "\n";
    return copy_file(backup_path, active);
}

bool OtaManager::health_check(ipc::RpmsgChannel &channel) {
    std::cout << "[OTA] Health-check: opening " << cfg_.rpmsg_device << "...\n";
    if (!channel.open()) {
        std::cerr << "[OTA] Cannot open RPMsg device after restart\n";
        return false;
    }

    ssize_t sent = channel.send_cmd(ipc::Cmd::Ping, 0);
    if (sent <= 0) {
        std::cerr << "[OTA] Health-check PING send failed\n";
        return false;
    }

    ipc::RespPacket resp{};
    if (!channel.receive_resp(resp)) {
        std::cerr << "[OTA] Health-check: no PING response from M33\n";
        return false;
    }

    if (resp.cmd != static_cast<uint8_t>(ipc::Cmd::Ping)
        || resp.status != 0x00U) {
        std::cerr << "[OTA] Health-check: unexpected response\n";
        return false;
    }

    std::cout << "[OTA] Health-check PASSED — M33 is alive\n";
    return true;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

Version OtaManager::get_version(ipc::RpmsgChannel &channel) {
    ssize_t sent = channel.send_cmd(ipc::Cmd::GetVersion, 0);
    if (sent <= 0) return {};

    ipc::RespPacket resp{};
    if (!channel.receive_resp(resp)) return {};
    if (resp.cmd != static_cast<uint8_t>(ipc::Cmd::GetVersion)) return {};

    return Version{resp.arg_lo, resp.arg_hi, resp.status};
}

Result OtaManager::update(const std::string &new_fw_path,
                           const std::string &sha256_path,
                           ipc::RpmsgChannel &channel) {
    std::string backup = cfg_.firmware_dir + "/" + cfg_.firmware_name + ".bak";

    /* ── 1. Verify the new firmware before touching anything ─────────────── */
    if (!file_exists(new_fw_path)) {
        std::cerr << "[OTA] New firmware not found: " << new_fw_path << "\n";
        return Result::VerifyFailed;
    }
    if (!verify_sha256(new_fw_path, sha256_path)) {
        return Result::VerifyFailed;
    }

    /* ── 2. Signal M33 to freeze IPC ─────────────────────────────────────── */
    if (!prepare_m33(channel)) {
        return Result::PrepareTimeout;
    }
    channel.close();  /* Done with IPC until M33 restarts */

    /* ── 3. Stop M33 via remoteproc ──────────────────────────────────────── */
    std::cout << "[OTA] Stopping M33 (remoteproc)...\n";
    if (!write_state("stop") || !wait_for_state("offline", cfg_.remoteproc_timeout_ms)) {
        return Result::StopFailed;
    }
    std::cout << "[OTA] M33 stopped\n";

    /* ── 4. Install new firmware ─────────────────────────────────────────── */
    if (!install_firmware(new_fw_path, backup)) {
        /* Try to start with whatever was there before */
        std::cerr << "[OTA] Install failed — attempting restart with old firmware\n";
        write_state("start");
        wait_for_state("running", cfg_.remoteproc_timeout_ms);
        return Result::InstallFailed;
    }

    /* ── 5. Start M33 with new firmware ──────────────────────────────────── */
    std::cout << "[OTA] Starting M33 with new firmware...\n";
    if (!write_state("start") || !wait_for_state("running", cfg_.remoteproc_timeout_ms)) {
        std::cerr << "[OTA] Start failed — rolling back\n";
        if (restore_firmware(backup)
            && write_state("start")
            && wait_for_state("running", cfg_.remoteproc_timeout_ms)) {
            return Result::RollbackOk;
        }
        return Result::RollbackFailed;
    }

    /* ── 6. Wait for /dev/ttyRPMSG0 to reappear ──────────────────────────── */
    std::cout << "[OTA] Waiting for RPMsg device...\n";
    if (!wait_for_device(cfg_.rpmsg_device, cfg_.rpmsg_appear_timeout_ms)) {
        std::cerr << "[OTA] RPMsg device did not appear — rolling back\n";
        write_state("stop");
        wait_for_state("offline", cfg_.remoteproc_timeout_ms);
        if (restore_firmware(backup)
            && write_state("start")
            && wait_for_state("running", cfg_.remoteproc_timeout_ms)) {
            return Result::RollbackOk;
        }
        return Result::RollbackFailed;
    }

    /* ── 7. Health-check: ping M33 via fresh RPMsg channel ───────────────── */
    ipc::RpmsgChannel health_ch(cfg_.rpmsg_device, cfg_.health_ping_timeout_ms);
    if (!health_check(health_ch)) {
        std::cerr << "[OTA] Health-check failed — rolling back\n";
        health_ch.close();
        write_state("stop");
        wait_for_state("offline", cfg_.remoteproc_timeout_ms);
        if (restore_firmware(backup)
            && write_state("start")
            && wait_for_state("running", cfg_.remoteproc_timeout_ms)) {
            return Result::RollbackOk;
        }
        return Result::RollbackFailed;
    }

    std::cout << "[OTA] Update complete\n";
    return Result::Ok;
}

}  // namespace ota
