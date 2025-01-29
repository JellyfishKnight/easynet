#include "websocket_utils.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

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

bool WebSocketFrame::rsv1() const {
    return m_rsv1;
}

bool WebSocketFrame::rsv2() const {
    return m_rsv2;
}

bool WebSocketFrame::rsv3() const {
    return m_rsv3;
}

bool WebSocketFrame::masked() const {
    return m_mask;
}

uint32_t WebSocketFrame::mask() const {
    return m_mask_key;
}

uint64_t WebSocketFrame::payload_length() const {
    if (m_payload_length_1 < 126) {
        return m_payload_length_1;
    } else if (m_payload_length_1 == 126) {
        return m_payload_length_2;
    } else {
        return m_payload_length_3;
    }
}

WebSocketFrame& WebSocketFrame::set_rsv1(bool rsv1) {
    m_rsv1 = rsv1;
    return *this;
}

WebSocketFrame& WebSocketFrame::set_rsv2(bool rsv2) {
    m_rsv2 = rsv2;
    return *this;
}

WebSocketFrame& WebSocketFrame::set_rsv3(bool rsv3) {
    m_rsv3 = rsv3;
    return *this;
}

WebSocketFrame& WebSocketFrame::set_mask(uint32_t mask) {
    m_mask_key = mask;
    m_mask = true;
    return *this;
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
    if (m_payload.size() < 126) {
        m_payload_length_1 = m_payload.size();
        m_payload_length_2 = 0;
        m_payload_length_3 = 0;
    } else if (m_payload.size() < 65536) {
        m_payload_length_1 = 126;
        m_payload_length_2 = m_payload.size();
        m_payload_length_3 = 0;
    } else {
        m_payload_length_1 = 127;
        m_payload_length_2 = 0;
        m_payload_length_3 = m_payload.size();
    }
    return *this;
}

WebSocketFrame& WebSocketFrame::append_payload(const std::string& payload) {
    m_payload += payload;
    if (m_payload.size() < 126) {
        m_payload_length_1 = m_payload.size();
        m_payload_length_2 = 0;
        m_payload_length_3 = 0;
    } else if (m_payload.size() < 65536) {
        m_payload_length_1 = 126;
        m_payload_length_2 = m_payload.size();
        m_payload_length_3 = 0;
    } else {
        m_payload_length_1 = 127;
        m_payload_length_2 = 0;
        m_payload_length_3 = m_payload.size();
    }
    return *this;
}

void WebSocketFrame::clear() {
    m_opcode = WebSocketOpcode::CONTINUATION;
    m_fin = false;
    m_payload.clear();
}

bool WebSocketFrame::is_control_frame() const {
    return m_opcode == WebSocketOpcode::CLOSE || m_opcode == WebSocketOpcode::PING
        || m_opcode == WebSocketOpcode::PONG;
}

std::string& websocket_parser::buffer_raw() {
    return m_buffer;
}

void websocket_parser::reset_state() {
    m_buffer.clear();
    m_frames = std::queue<WebSocketFrame>();
    m_finished_frame = false;
}

bool websocket_parser::has_finished_frame() const {
    return m_finished_frame;
}

std::optional<WebSocketFrame> websocket_parser::read_frame() {
    if (m_frames.empty()) {
        return std::nullopt;
    }
    WebSocketFrame frame = m_frames.front();
    m_frames.pop();
    return frame;
}

void websocket_parser::push_chunk(const std::string& chunk) {
    /// TODO: Implement this
    if (!m_finished_frame) {
        m_buffer.append(chunk);
        if (m_buffer.size() < 2) {
            return;
        }
        uint8_t byte1 = m_buffer[offset];
        uint8_t byte2 = m_buffer[offset + 1];
        WebSocketFrame frame;
        frame.set_fin(byte1 & 0x80)
            .set_rsv1(byte1 & 0x40)
            .set_rsv2(byte1 & 0x20)
            .set_rsv3(byte1 & 0x10)
            .set_opcode(static_cast<WebSocketOpcode>(byte1 & 0x0F))
            .set_mask(byte2 & 0x80);
        uint8_t payload_length = byte2 & 0x7F;
        uint64_t length = 0;
        uint8_t head_size = 2;
        if (payload_length == 126) {
            if (m_buffer.size() < 4) {
                return;
            }
            length = (m_buffer[2] << 8) | m_buffer[3];
            head_size = 4;
        } else {
            if (m_buffer.size() < 10) {
                return;
            }
            for (int i = 2; i < 10; ++i) {
                length = (length << 8) | m_buffer[i];
            }
            head_size = 10;
        }
        if (frame.masked()) {
            if (m_buffer.size() < 14) {
                return;
            }
            frame.set_mask(
                (m_buffer[10] << 24) | (m_buffer[11] << 16) | (m_buffer[12] << 8) | m_buffer[13]
            );
        }
        frame.set_payload(m_buffer.substr(head_size, length));
        m_buffer = m_buffer.substr(head_size + length);
        m_frames.push(frame);
        m_finished_frame = true;
        if (m_buffer.empty()) {
            m_finished_frame = false;
        }
    }
}

std::string& websocket_writer::buffer() {
    return m_buffer;
}

void websocket_writer::reset_state() {
    m_buffer.clear();
}

void websocket_writer::write_frame(const WebSocketFrame& frame) {
    std::size_t head_size = 0;
    m_buffer.resize(1024);
    m_buffer[0] |= frame.fin() << 8;
    m_buffer[0] |= frame.rsv1() << 7;
    m_buffer[0] |= frame.rsv2() << 6;
    m_buffer[0] |= frame.rsv3() << 5;
    m_buffer[0] |= static_cast<uint8_t>(frame.opcode());
    m_buffer[1] |= frame.masked() << 7;
    m_buffer[1] |= frame.payload_length() < 126 ? frame.payload_length() : 126;
    if (frame.payload_length() == 126) {
        m_buffer[2] = frame.payload_length() >> 8;
        m_buffer[3] = frame.payload_length();
        head_size = 4;
    } else if (frame.payload_length() == 127) {
        m_buffer[2] = frame.payload_length() >> 56;
        m_buffer[3] = frame.payload_length() >> 48;
        m_buffer[4] = frame.payload_length() >> 40;
        m_buffer[5] = frame.payload_length() >> 32;
        m_buffer[6] = frame.payload_length() >> 24;
        m_buffer[7] = frame.payload_length() >> 16;
        m_buffer[8] = frame.payload_length() >> 8;
        m_buffer[9] = frame.payload_length();
        head_size = 10;
    } else {
        head_size = 2;
    }
    if (frame.masked()) {
        m_buffer[head_size] = frame.mask() >> 24;
        m_buffer[head_size + 1] = frame.mask() >> 16;
        m_buffer[head_size + 2] = frame.mask() >> 8;
        m_buffer[head_size + 3] = frame.mask();
        head_size += 4;
    }
    m_buffer.resize(head_size);
    m_buffer.append(frame.payload());
}

std::vector<uint8_t> WebSocketParser::write_frame(const WebSocketFrame& frame) {
    m_writer.reset_state();
    m_writer.write_frame(frame);
    return std::vector<uint8_t>(m_writer.buffer().begin(), m_writer.buffer().end());
}

std::optional<WebSocketFrame> WebSocketParser::read_frame(const std::vector<uint8_t>& data) {
    std::string chunk(data.begin(), data.end());
    m_parser.push_chunk(chunk);
    if (m_parser.has_finished_frame()) {
        return m_parser.read_frame();
    }
    return std::nullopt;
}

} // namespace net