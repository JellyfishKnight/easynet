#include "http_parser.hpp"
#include "enum_parser.hpp"
#include <cassert>

namespace net {

HttpResponse& HttpResponse::set_version(const std::string& version) {
    m_version = version;
    return *this;
}

HttpResponse& HttpResponse::set_status_code(HttpResponseCode status_code) {
    m_status_code = status_code;
    return *this;
}

HttpResponse& HttpResponse::set_reason(const std::string& reason) {
    m_reason = std::move(reason);
    return *this;
}

HttpResponse& HttpResponse::set_header(const std::string& key, const std::string& value) {
    m_headers.insert_or_assign(key, value);
    return *this;
}

HttpResponse& HttpResponse::set_headers(const std::unordered_map<std::string, std::string>& headers) {
    m_headers = headers;
    return *this;
}

HttpResponse& HttpResponse::set_body(const std::string& body) {
    m_body = body;
    return *this;
}

[[nodiscard]] const std::string& HttpResponse::version() const {
    return m_version;
}

[[nodiscard]] HttpResponseCode HttpResponse::status_code() const {
    return m_status_code;
}

[[nodiscard]] const std::string& HttpResponse::reason() const {
    return m_reason;
}

[[nodiscard]] const std::string& HttpResponse::header(const std::string& key) const {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return std::move(std::string());
    }
    return it->second;
}

[[nodiscard]] const std::unordered_map<std::string, std::string>& HttpResponse::headers() const {
    return m_headers;
}

[[nodiscard]] const std::string& HttpResponse::body() const {
    return m_body;
}

HttpRequest& HttpRequest::set_version(const std::string& version) {
    m_version = version;
    return *this;
}

HttpRequest& HttpRequest::set_url(const std::string& url) {
    m_url = url;
    return *this;
}

HttpRequest& HttpRequest::set_header(const std::string& key, const std::string& value) {
    m_headers.insert_or_assign(key, value);
    return *this;
}

HttpRequest& HttpRequest::set_headers(const std::unordered_map<std::string, std::string>& headers) {
    m_headers = headers;
    return *this;
}

HttpRequest& HttpRequest::set_body(const std::string& body) {
    m_body = body;
    return *this;
}

HttpRequest& HttpRequest::set_method(HttpMethod method) {
    m_method = method;
    return *this;
}

[[nodiscard]] HttpMethod HttpRequest::method() const {
    return m_method;
}

[[nodiscard]] const std::string& HttpRequest::url() const {
    return m_url;
}

[[nodiscard]] const std::string& HttpRequest::version() const {
    return m_version;
}

[[nodiscard]] const std::string& HttpRequest::header(const std::string& key) const {
    auto it = m_headers.find(key);
    if (it == m_headers.end()) {
        return std::move(std::string());
    }
    return it->second;
}

[[nodiscard]] const std::unordered_map<std::string, std::string>& HttpRequest::headers() const {
    return m_headers;
}

[[nodiscard]] const std::string& HttpRequest::body() const {
    return m_body;
}

void http11_header_parser::reset_state() {
    m_header.clear();
    m_headline.clear();
    m_header_keys.clear();
    m_body.clear();
    m_header_finished = 0;
}

[[nodiscard]] bool http11_header_parser::header_finished() {
    return m_header_finished; // 如果正文都结束了，就不再需要更多数据
}

void http11_header_parser::_extract_headers() {
    std::string_view header = m_header;
    size_t pos = header.find("\r\n", 0, 2);
    m_headline = std::string(header.substr(0, pos));
    while (pos != std::string::npos) {
        // 跳过 "\r\n"
        pos += 2;
        // 从当前位置开始找，先找到下一行位置（可能为 npos）
        size_t next_pos = header.find("\r\n", pos, 2);
        size_t line_len = std::string::npos;
        if (next_pos != std::string::npos) {
            // 如果下一行还不是结束，那么 line_len
            // 设为本行开始到下一行之间的距离
            line_len = next_pos - pos;
        }
        // 就能切下本行
        std::string_view line = header.substr(pos, line_len);
        size_t colon = line.find(": ", 0, 2);
        if (colon != std::string::npos) {
            // 每一行都是 "键: 值"
            std::string key = std::string(line.substr(0, colon));
            std::string_view value = line.substr(colon + 2);
            // 键统一转成小写，实现大小写不敏感的判断
            for (char& c: key) {
                if ('A' <= c && c <= 'Z') {
                    c += 'a' - 'A';
                }
            }
            // 古代 C++ 过时的写法：m_header_keys[key] = value;
            // 现代 C++17 的高效写法：
            m_header_keys.insert_or_assign(std::move(key), value);
        }
        pos = next_pos;
    }
}

void http11_header_parser::push_chunk(std::string& chunk) {
    assert(!m_header_finished);
    size_t old_size = m_header.size();
    m_header.append(chunk);
    std::string_view header = m_header;
    // 如果还在解析头部的话，尝试判断头部是否结束
    if (old_size < 4) {
        old_size = 4;
    }
    old_size -= 4;
    size_t header_len = header.find("\r\n\r\n", old_size, 4);
    if (header_len != std::string::npos) {
        // 头部已经结束
        m_header_finished = true;
        // 把不小心多读取的正文留下
        m_header.resize(header_len);
        // 开始分析头部，尝试提取 Content-length 字段
        _extract_headers();
        auto content_length = m_header_keys.find("content-length");
        if (content_length != m_header_keys.end()) {
            size_t body_len = std::stoul(content_length->second);
            if (header_len + 4 + body_len <= header.size()) {
                m_body = header.substr(header_len + 4, body_len);
                chunk = chunk.substr(header_len + 4 + body_len);
            } else {
                m_body = header.substr(header_len + 4, header.size() - header_len - 4);
                chunk.clear();
            }
        } else {
            chunk = chunk.substr(header_len + 4);
        }
    }
}

std::string& http11_header_parser::headline() {
    return m_headline;
}

std::unordered_map<std::string, std::string>& http11_header_parser::headers() {
    return m_header_keys;
}

std::string& http11_header_parser::headers_raw() {
    return m_header;
}

std::string& http11_header_parser::extra_body() {
    return m_body;
}

void http11_header_writer::reset_state() {
    m_buffer.clear();
}

std::string& http11_header_writer::buffer() {
    return m_buffer;
}

void http11_header_writer::begin_header(std::string_view first, std::string_view second, std::string_view third) {
    m_buffer.append(first);
    m_buffer.append(" ");
    m_buffer.append(second);
    m_buffer.append(" ");
    m_buffer.append(third);
}

void http11_header_writer::write_header(std::string_view key, std::string_view value) {
    m_buffer.append("\r\n");
    m_buffer.append(key);
    m_buffer.append(": ");
    m_buffer.append(value);
}

void http11_header_writer::end_header() {
    m_buffer.append("\r\n\r\n");
}

std::vector<uint8_t> HttpParser::write_req(const HttpRequest& req) {
    m_req_writer.reset_state();
    m_req_writer.begin_header(utils::dump_enum(req.method()), req.url());
    for (auto& [key, value]: req.headers()) {
        m_req_writer.write_header(key, value);
    }
    m_req_writer.end_header();
    m_req_writer.write_body(req.body());
    return std::vector<uint8_t>(m_req_writer.buffer().begin(), m_req_writer.buffer().end());
}

std::vector<uint8_t> HttpParser::write_res(const HttpResponse& res) {
    m_res_writer.reset_state();
    m_res_writer.begin_header(static_cast<int>(res.status_code()));
    for (auto& [key, value]: res.headers()) {
        m_res_writer.write_header(key, value);
    }
    if (!res.body().empty() && !res.headers().contains("Content-Length")) {
        m_res_writer.write_header("Content-Length", std::to_string(res.body().size()));
    }
    m_res_writer.end_header();
    m_res_writer.write_body(res.body());
    return std::vector<uint8_t>(m_res_writer.buffer().begin(), m_res_writer.buffer().end());
}

std::optional<HttpRequest> HttpParser::read_req() {
    if (m_req_parser.request_finished()) {
        HttpRequest req;
        req.set_method(m_req_parser.method())
            .set_url(m_req_parser.url())
            .set_version(m_req_parser.version())
            .set_headers(m_req_parser.headers())
            .set_body(m_req_parser.body());
        m_req_parser.reset_state();
        add_req_read_buffer({});
        return req;
    }
    return std::nullopt;
}

void HttpParser::add_req_read_buffer(const std::vector<uint8_t>& buffer) {
    m_req_read_buffer.insert(m_req_read_buffer.end(), buffer.begin(), buffer.end());
    m_req_parser.push_chunk(m_req_read_buffer);
}

std::optional<HttpResponse> HttpParser::read_res() {
    if (m_res_parser.request_finished()) {
        HttpResponse res;
        res.set_version(m_res_parser.version())
            .set_status_code(static_cast<HttpResponseCode>(m_res_parser.status()))
            .set_reason(std::string(utils::dump_enum(res.status_code())))
            .set_headers(m_res_parser.headers())
            .set_body(m_res_parser.body());
        m_res_parser.reset_state();
        add_res_read_buffer({});
        return res;
    }
    return std::nullopt;
}

void HttpParser::add_res_read_buffer(const std::vector<uint8_t>& buffer) {
    m_res_read_buffer.insert(m_res_read_buffer.end(), buffer.begin(), buffer.end());
    m_res_parser.push_chunk(m_res_read_buffer);
}

} // namespace net