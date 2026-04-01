/**
 * @file  ipc_rpmsg.h
 * @brief RPMsg IPC client running on the Cortex-A35 (Linux userspace).
 *
 * Opens /dev/ttyRPMSGx exposed by the Linux rpmsg_tty kernel driver,
 * which provides a character-device interface to the OpenAMP RPMsg
 * channel connecting to the Cortex-M33.
 *
 * Protocol (matches ipc_handler.h on the M33 side)
 * ─────────────────────────────────────────────────
 * Command  (A35 → M33, 5 bytes):  [0xAA][CMD][ARG_LO][ARG_HI][0x55]
 * Response (M33 → A35, 8 bytes):  [0xBB][CMD][ARG_LO][ARG_HI][ST][T0][T1][0x55]
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace ipc {

/* ── Frame markers ──────────────────────────────────────────────────────── */
inline constexpr uint8_t SOF_CMD  = 0xAAU;
inline constexpr uint8_t SOF_RESP = 0xBBU;
inline constexpr uint8_t EOF_MARK = 0x55U;

/* ── Command identifiers ────────────────────────────────────────────────── */
enum class Cmd : uint8_t {
    Ping       = 0x01,
    LedSet     = 0x02,
    LedBlink   = 0x03,
    GetStatus  = 0x04,
    GetVersion = 0x05,  /**< ARG_LO=major, ARG_HI=minor, STATUS=patch */
    OtaPrepare = 0x06,  /**< Ask M33 to freeze IPC before remoteproc stop */
};

/* ── Response status codes ──────────────────────────────────────────────── */
enum class Status : uint8_t {
    Ok      = 0x00,
    Err     = 0xFF,
    Unknown = 0xFE,
};

/* ── Typed packet structs ────────────────────────────────────────────────── */
#pragma pack(push, 1)
struct CmdPacket {
    uint8_t  sof;     /**< IPC_SOF_CMD = 0xAA */
    uint8_t  cmd;     /**< ipc::Cmd */
    uint8_t  arg_lo;
    uint8_t  arg_hi;
    uint8_t  eof;     /**< IPC_EOF = 0x55 */
};

struct RespPacket {
    uint8_t  sof;     /**< IPC_SOF_RESP = 0xBB */
    uint8_t  cmd;     /**< ipc::Cmd echoed */
    uint8_t  arg_lo;
    uint8_t  arg_hi;
    uint8_t  status;  /**< ipc::Status */
    uint8_t  tick_lo; /**< HAL_GetTick() low byte */
    uint8_t  tick_hi; /**< HAL_GetTick() high byte */
    uint8_t  eof;     /**< IPC_EOF = 0x55 */
};
#pragma pack(pop)

/* ── Helper to build a command packet ───────────────────────────────────── */
inline CmdPacket make_cmd(Cmd cmd, uint16_t arg = 0) {
    return CmdPacket{
        SOF_CMD,
        static_cast<uint8_t>(cmd),
        static_cast<uint8_t>(arg & 0xFFU),
        static_cast<uint8_t>((arg >> 8U) & 0xFFU),
        EOF_MARK
    };
}

/* ── RPMsg channel wrapper ───────────────────────────────────────────────── */
class RpmsgChannel {
public:
    /**
     * @param device  Path to the virtual serial device, e.g. "/dev/ttyRPMSG0"
     * @param timeout_ms  Receive timeout in milliseconds (0 = blocking)
     */
    explicit RpmsgChannel(std::string_view device, int timeout_ms = 2000);
    ~RpmsgChannel();

    RpmsgChannel(const RpmsgChannel&)            = delete;
    RpmsgChannel& operator=(const RpmsgChannel&) = delete;

    /** Open the channel. Returns true on success. */
    bool open();

    /** Close the channel. */
    void close();

    bool is_open() const { return fd_ >= 0; }

    /**
     * Send a command packet to the M33.
     * @return Number of bytes sent, or -1 on error.
     */
    ssize_t send_cmd(Cmd cmd, uint16_t arg = 0);

    /**
     * Receive a response packet from the M33 (blocking up to timeout_ms).
     * @param resp  Output struct to fill.
     * @return true if a valid response was received.
     */
    bool receive_resp(RespPacket &resp);

    /** Low-level byte send (for raw payloads). */
    ssize_t send(const uint8_t *data, size_t length);

    /** Low-level byte receive. */
    ssize_t receive(uint8_t *buf, size_t length);

private:
    std::string device_;
    int         fd_{-1};
    int         timeout_ms_;
};

}  // namespace ipc
