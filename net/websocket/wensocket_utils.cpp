#include "websocket_utils.hpp"

namespace net {

std::string base64_encode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input.data(), input.size());
    BIO_flush(b64);
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(b64, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(b64);
    return result;
}

std::string websocket_accept_key(const std::string& client_key) {
    static const std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concat = client_key + magicString;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(concat.c_str()), concat.size(), hash);
    return base64_encode(hash, SHA_DIGEST_LENGTH);
}

WebSocketFrame::WebSocketFrame(WebSocketOpcode opcode, const std::string& payload, bool fin) {
    m_opcode = opcode;
    m_payload = payload;
    m_fin = fin;
}

WebSocketOpcode WebSocketFrame::opcode() const {
    return m_opcode;
}

bool WebSocketFrame::fin() const {
    return m_fin;
}

const std::string& WebSocketFrame::payload() const {
    return m_payload;
}

WebSocketFrame& WebSocketFrame::set_opcode(WebSocketOpcode opcode) {
    m_opcode = opcode;
    return *this;
}

WebSocketFrame& WebSocketFrame::set_fin(bool fin) {
    m_fin = fin;
    return *this;
}

WebSocketFrame& WebSocketFrame::set_payload(const std::string& payload) {
    m_payload = payload;
    return *this;
}

WebSocketFrame& WebSocketFrame::append_payload(const std::string& payload) {
    m_payload += payload;
    return *this;
}

void WebSocketFrame::clear() {
    m_opcode = WebSocketOpcode::CONTINUATION;
    m_fin = false;
    m_payload.clear();
}

bool WebSocketFrame::is_control_frame() const {
    return m_opcode == WebSocketOpcode::CLOSE
        || m_opcode == WebSocketOpcode::PING
        || m_opcode == WebSocketOpcode::PONG;
}

} // namespace net