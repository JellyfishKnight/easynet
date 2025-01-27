#pragma once

#include <cstdint>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <optional>
#include <queue>
#include <string>
#include <sys/types.h>
#include <vector>

namespace net {

std::string base64_encode(const unsigned char* input, size_t length);

std::string websocket_accept_key(const std::string& client_key);

enum class WebSocketOpcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

enum class WebSocketCloseCode {
    NORMAL = 1000,
    GOING_AWAY = 1001,
    PROTOCOL_ERROR = 1002,
    UNSUPPORTED_DATA = 1003,
    NO_STATUS_RCVD = 1005,
    ABNORMAL_CLOSURE = 1006,
    INVALID_FRAME_PAYLOAD_DATA = 1007,
    POLICY_VIOLATION = 1008,
    MESSAGE_TOO_BIG = 1009,
    MISSING_EXTENSION = 1010,
    INTERNAL_ERROR = 1011,
    SERVICE_RESTART = 1012,
    TRY_AGAIN_LATER = 1013,
    BAD_GATEWAY = 1014,
    TLS_HANDSHAKE = 1015
};

struct WebSocketFrame {
public:
    WebSocketFrame() = default;

    WebSocketFrame(WebSocketOpcode opcode, const std::string& payload, bool fin);

    ~WebSocketFrame() = default;

    WebSocketOpcode opcode() const;
    bool fin() const;
    const std::string& payload() const;
    bool rsv1() const;
    bool rsv2() const;
    bool rsv3() const;
    bool masked() const;
    uint32_t mask() const;
    uint64_t payload_length() const;

    WebSocketFrame& set_rsv1(bool rsv1);
    WebSocketFrame& set_rsv2(bool rsv2);
    WebSocketFrame& set_rsv3(bool rsv3);
    WebSocketFrame& set_mask(uint32_t mask);
    WebSocketFrame& set_opcode(WebSocketOpcode opcode);
    WebSocketFrame& set_fin(bool fin);
    WebSocketFrame& set_payload(const std::string& payload);
    WebSocketFrame& append_payload(const std::string& payload);

    void clear();

    bool is_control_frame() const;

private:
    WebSocketOpcode m_opcode;
    bool m_fin;
    bool m_rsv1;
    bool m_rsv2;
    bool m_rsv3;

    uint8_t m_payload_length_1;
    uint16_t m_payload_length_2;
    uint64_t m_payload_length_3;

    bool m_mask;
    uint32_t m_mask_key = false;
    std::string m_payload;
};

struct websocket_parser {
    std::string m_buffer;
    std::queue<WebSocketFrame> m_frames;

    void push_chunk(const std::string& chunk);

    void reset_state();

    std::string& buffer_raw();

    std::optional<WebSocketFrame> read_frame();
};

struct websocket_writer {
    std::string m_buffer;

    void reset_state();

    std::string& buffer();

    std::string write_frame(const WebSocketFrame& frame);
};

class WebSocketParser {
private:
    websocket_parser m_parser;
    websocket_writer m_writer;

public:
    WebSocketParser() = default;

    std::vector<uint8_t> write_frame(const WebSocketFrame& frame);

    std::optional<WebSocketFrame> read_frame(const std::vector<uint8_t>& data);

    void reset_state();

    bool has_finished_frame();
};

} // namespace net