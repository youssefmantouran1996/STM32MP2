/**
 * @file  ipc_rpmsg.h
 * @brief RPMsg IPC client running on the Cortex-A35 (Linux userspace).
 *
 * Opens the /dev/ttyRPMSGx virtual serial device exposed by the kernel
 * rpmsg_tty driver, which provides a character-device interface to the
 * OpenAMP RPMsg channel connecting to the Cortex-M33.
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace ipc {

class RpmsgChannel {
public:
    /**
     * @param device  Path to the virtual serial device, e.g. "/dev/ttyRPMSG0"
     */
    explicit RpmsgChannel(std::string_view device);
    ~RpmsgChannel();

    /* Non-copyable */
    RpmsgChannel(const RpmsgChannel&)            = delete;
    RpmsgChannel& operator=(const RpmsgChannel&) = delete;

    /** Open the channel. Returns true on success. */
    bool open();

    /** Close the channel. */
    void close();

    /**
     * Send a message to the M33.
     * @param data   Pointer to payload bytes
     * @param length Payload length (max RPMSG_MAX_PAYLOAD bytes)
     * @return Number of bytes sent, or -1 on error.
     */
    ssize_t send(const uint8_t* data, size_t length);

    /**
     * Receive a message from the M33 (blocking).
     * @param buf    Output buffer
     * @param length Buffer size
     * @return Number of bytes received, or -1 on error.
     */
    ssize_t receive(uint8_t* buf, size_t length);

    bool is_open() const { return fd_ >= 0; }

private:
    std::string device_;
    int fd_{-1};
};

}  // namespace ipc
