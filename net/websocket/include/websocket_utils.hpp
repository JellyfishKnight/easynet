#pragma once

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string>

namespace net {

std::string base64_encode(const unsigned char* input, size_t length);

std::string websocket_accept_key(const std::string& client_key);

enum class WebSocketOpcode {
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

class WebSocketFrame {
public:
    WebSocketFrame(WebSocketOpcode opcode, const std::string& payload, bool fin = true);
    WebSocketFrame(
        WebSocketOpcode opcode,
        const unsigned char* payload,
        size_t length,
        bool fin = true
    );
    WebSocketFrame(WebSocketOpcode opcode, bool fin = true);
    WebSocketFrame(const WebSocketFrame& other);
    WebSocketFrame(WebSocketFrame&& other);
    ~WebSocketFrame();

    WebSocketFrame& operator=(const WebSocketFrame& other);
    WebSocketFrame& operator=(WebSocketFrame&& other);

    WebSocketOpcode opcode() const;
    bool fin() const;
    const std::string& payload() const;
    const unsigned char* payloadData() const;
    size_t payloadLength() const;

    void set_opcode(WebSocketOpcode opcode);
    void set_fin(bool fin);
    void set_payload(const std::string& payload);

    void append_payload(const std::string& payload);

    void clear();

    bool is_control_frame() const;
};

struct websocket_parser {
    std::string m_buffer;

    void push_chunk(const std::string& chunk);

    void reset_state();


};

struct websocket_writer {};

class WebSocketParser {};

} // namespace net