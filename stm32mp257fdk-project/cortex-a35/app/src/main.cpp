/**
 * @file  main.cpp
 * @brief STM32MP257FDK — Cortex-A35 Linux userspace application.
 *
 * Demonstrates bidirectional communication with the Cortex-M33 coprocessor
 * via the RPMsg virtual UART channel (/dev/ttyRPMSG0).
 *
 * Prerequisites on the board:
 *   - OpenSTLinux running on the A35
 *   - M33 firmware loaded via: echo firmware.elf > /sys/class/remoteproc/remoteproc0/firmware
 *                               echo start > /sys/class/remoteproc/remoteproc0/state
 */

#include "ipc_rpmsg.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>

static constexpr const char* RPMSG_DEV = "/dev/ttyRPMSG0";

int main() {
    std::cout << "STM32MP257FDK Cortex-A35 application starting...\n";

    ipc::RpmsgChannel channel(RPMSG_DEV);
    if (!channel.open()) {
        std::cerr << "Failed to open IPC channel. Is the M33 firmware running?\n";
        return 1;
    }

    /* Simple ping-pong loop */
    std::array<uint8_t, 16> tx_buf;
    std::array<uint8_t, 256> rx_buf;
    uint32_t counter = 0;

    while (true) {
        /* Build a simple 4-byte command: [0xAA, counter_lo, counter_hi, 0x55] */
        tx_buf[0] = 0xAA;
        tx_buf[1] = static_cast<uint8_t>(counter & 0xFF);
        tx_buf[2] = static_cast<uint8_t>((counter >> 8) & 0xFF);
        tx_buf[3] = 0x55;

        ssize_t sent = channel.send(tx_buf.data(), 4);
        if (sent > 0) {
            std::cout << "[A35 → M33] Sent " << sent << " bytes (counter=" << counter << ")\n";
        }

        ssize_t received = channel.receive(rx_buf.data(), rx_buf.size());
        if (received > 0) {
            std::cout << "[M33 → A35] Received " << received << " bytes\n";
        }

        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
