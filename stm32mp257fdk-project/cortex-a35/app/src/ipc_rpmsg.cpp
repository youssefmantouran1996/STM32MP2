/**
 * @file  ipc_rpmsg.cpp
 * @brief RPMsg IPC client — Cortex-A35 Linux userspace implementation.
 */

#include "ipc_rpmsg.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <unistd.h>

namespace ipc {

RpmsgChannel::RpmsgChannel(std::string_view device, int timeout_ms)
    : device_(device), timeout_ms_(timeout_ms) {}

RpmsgChannel::~RpmsgChannel() {
    close();
}

bool RpmsgChannel::open() {
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) {
        std::cerr << "[IPC] Failed to open " << device_
                  << ": " << std::strerror(errno) << "\n";
        return false;
    }
    std::cout << "[IPC] Opened RPMsg channel: " << device_ << "\n";
    return true;
}

void RpmsgChannel::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

ssize_t RpmsgChannel::send(const uint8_t *data, size_t length) {
    if (fd_ < 0) return -1;
    ssize_t written = ::write(fd_, data, length);
    if (written < 0) {
        std::cerr << "[IPC] Write error: " << std::strerror(errno) << "\n";
    }
    return written;
}

ssize_t RpmsgChannel::receive(uint8_t *buf, size_t length) {
    if (fd_ < 0) return -1;

    if (timeout_ms_ > 0) {
        struct pollfd pfd{fd_, POLLIN, 0};
        int ret = ::poll(&pfd, 1, timeout_ms_);
        if (ret <= 0) {
            if (ret == 0) {
                std::cerr << "[IPC] Receive timeout (" << timeout_ms_ << " ms)\n";
            } else {
                std::cerr << "[IPC] Poll error: " << std::strerror(errno) << "\n";
            }
            return -1;
        }
    }

    ssize_t n = ::read(fd_, buf, length);
    if (n < 0) {
        std::cerr << "[IPC] Read error: " << std::strerror(errno) << "\n";
    }
    return n;
}

ssize_t RpmsgChannel::send_cmd(Cmd cmd, uint16_t arg) {
    CmdPacket pkt = make_cmd(cmd, arg);
    return send(reinterpret_cast<const uint8_t *>(&pkt), sizeof(pkt));
}

bool RpmsgChannel::receive_resp(RespPacket &resp) {
    ssize_t n = receive(reinterpret_cast<uint8_t *>(&resp), sizeof(resp));
    if (n != static_cast<ssize_t>(sizeof(resp))) {
        return false;
    }
    /* Validate frame markers */
    if (resp.sof != SOF_RESP || resp.eof != EOF_MARK) {
        std::cerr << "[IPC] Bad response frame (sof=0x" << std::hex
                  << static_cast<int>(resp.sof) << " eof=0x"
                  << static_cast<int>(resp.eof) << std::dec << ")\n";
        return false;
    }
    return true;
}

}  // namespace ipc
