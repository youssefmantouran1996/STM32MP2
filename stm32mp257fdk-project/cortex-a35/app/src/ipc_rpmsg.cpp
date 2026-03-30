/**
 * @file  ipc_rpmsg.cpp
 * @brief RPMsg IPC client — Cortex-A35 Linux userspace implementation.
 */

#include "ipc_rpmsg.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace ipc {

RpmsgChannel::RpmsgChannel(std::string_view device) : device_(device) {}

RpmsgChannel::~RpmsgChannel() {
    close();
}

bool RpmsgChannel::open() {
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ < 0) {
        std::cerr << "[IPC] Failed to open " << device_ << ": " << std::strerror(errno) << "\n";
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

ssize_t RpmsgChannel::send(const uint8_t* data, size_t length) {
    if (fd_ < 0) return -1;
    ssize_t written = ::write(fd_, data, length);
    if (written < 0) {
        std::cerr << "[IPC] Write error: " << std::strerror(errno) << "\n";
    }
    return written;
}

ssize_t RpmsgChannel::receive(uint8_t* buf, size_t length) {
    if (fd_ < 0) return -1;
    ssize_t n = ::read(fd_, buf, length);
    if (n < 0) {
        std::cerr << "[IPC] Read error: " << std::strerror(errno) << "\n";
    }
    return n;
}

}  // namespace ipc
