/**
 * @file  main.cpp
 * @brief STM32MP257FDK — Cortex-A35 Linux userspace application.
 *
 * Communicates with the Cortex-M33 coprocessor via the RPMsg virtual UART
 * channel (/dev/ttyRPMSG0).
 *
 * Prerequisites on the board:
 *   - OpenSTLinux running on the A35
 *   - M33 firmware loaded and started via remoteproc:
 *       echo stm32mp257fdk_m33 > /sys/class/remoteproc/remoteproc0/firmware
 *       echo start             > /sys/class/remoteproc/remoteproc0/state
 *   - /dev/ttyRPMSG0 appears after the M33 sends the NS announcement
 *
 * Usage:
 *   ./stm32mp257fdk_app [/dev/ttyRPMSG0]
 */

#include "ipc_rpmsg.h"
#include "virt_uart.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

/* ── Helper: decode and print a response packet ─────────────────────────── */
static void print_response(const ipc::RespPacket &r, uint32_t counter) {
    uint16_t tick = static_cast<uint16_t>(r.tick_lo)
                  | static_cast<uint16_t>(static_cast<uint16_t>(r.tick_hi) << 8U);

    std::cout << "[M33→A35] #" << std::setw(4) << counter
              << "  cmd=0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(r.cmd)
              << "  status=" << (r.status == static_cast<uint8_t>(ipc::Status::Ok)
                                 ? "OK " : "ERR")
              << "  m33_tick=" << std::dec << tick << " ms"
              << "\n";
}

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    const char *dev = (argc > 1) ? argv[1] : "/dev/" VIRT_UART_CH0_NAME "0";

    std::cout << "STM32MP257FDK A35 application\n";
    std::cout << "  Channel : " << dev << "\n";
    std::cout << "  Protocol: cmd=[0xAA CMD AL AH 0x55]  "
                 "resp=[0xBB CMD AL AH ST T0 T1 0x55]\n\n";

    ipc::RpmsgChannel channel(dev, /*timeout_ms=*/2000);
    if (!channel.open()) {
        std::cerr << "Failed to open IPC channel.\n"
                  << "Is the M33 firmware running?\n"
                  << "  echo start > /sys/class/remoteproc/remoteproc0/state\n";
        return EXIT_FAILURE;
    }

    uint32_t counter = 0;

    while (true) {
        /* Cycle through commands to exercise all M33 handlers */
        ipc::Cmd cmd;
        uint16_t arg = 0;

        switch (counter % 4) {
            case 0:
                cmd = ipc::Cmd::Ping;
                break;
            case 1:
                cmd = ipc::Cmd::LedSet;
                arg = 1U;  /* LED on */
                break;
            case 2:
                cmd = ipc::Cmd::LedBlink;
                arg = 250U;  /* 250 ms blink */
                break;
            default:
                cmd = ipc::Cmd::GetStatus;
                break;
        }

        ssize_t sent = channel.send_cmd(cmd, arg);
        if (sent > 0) {
            std::cout << "[A35→M33] #" << std::setw(4) << counter
                      << "  cmd=0x" << std::hex << std::setw(2)
                      << std::setfill('0') << static_cast<int>(cmd)
                      << "  arg=" << std::dec << arg << "\n";
        } else {
            std::cerr << "[A35→M33] Send failed\n";
        }

        ipc::RespPacket resp{};
        if (channel.receive_resp(resp)) {
            print_response(resp, counter);
        } else {
            std::cerr << "[M33→A35] No response (M33 may be busy or not running)\n";
        }

        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return EXIT_SUCCESS;
}
