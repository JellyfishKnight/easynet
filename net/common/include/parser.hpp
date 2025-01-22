#pragma once

#include "enum_parser.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace net {

template<typename ReqType, typename ResType>
class BaseParser: std::enable_shared_from_this<BaseParser<ReqType, ResType>> {
public:
    virtual std::vector<uint8_t> write_req(const ReqType& req) = 0;

    virtual std::vector<uint8_t> write_res(const ResType& res) = 0;

    virtual void read_req(std::vector<uint8_t>& req, ReqType& req_out) = 0;

    virtual void read_res(std::vector<uint8_t>& res, ResType& res_out) = 0;

    virtual bool req_read_finished() = 0;

    virtual bool res_read_finished() = 0;
};

enum class HttpMethod {
    UNKNOWN = -1,
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    TRACE,
    CONNECT,
};

enum class HttpResponseCode {
    UNKNOWN = -1,
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,
    EARLY_HINTS = 103,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,
    ALREADY_REPORTED = 208,
    IM_USED = 226,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    IM_A_TEAPOT = 418,
    MISDIRECTED_REQUEST = 421,
    UNPROCESSABLE_ENTITY = 422,
    LOCKED = 423,
    FAILED_DEPENDENCY = 424,
    TOO_EARLY = 425,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 428,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    UNAVAILABLE_FOR_LEGAL_REASONS = 451,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 507,
    LOOP_DETECTED = 508,
    NOT_EXTENDED = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511,
};

struct HttpResponse {
    std::string version;
    HttpResponseCode status_code;
    std::string reason;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpRequest {
    HttpMethod method;
    std::string url;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct http11_header_parser {
    std::string m_header; // "GET / HTTP/1.1\nHost: 142857.red\r\nAccept:
        // */*\r\nConnection: close"
    std::string m_headline; // "GET / HTTP/1.1"
    std::unordered_map<std::string, std::string>
        m_header_keys; // {"Host": "142857.red", "Accept": "*/*",
    // "Connection: close"}
    std::string m_body; // 不小心超量读取的正文（如果有的话）
    bool m_header_finished = false;

    void reset_state() {
        m_header.clear();
        m_headline.clear();
        m_header_keys.clear();
        m_body.clear();
        m_header_finished = 0;
    }

    [[nodiscard]] bool header_finished() {
        return m_header_finished; // 如果正文都结束了，就不再需要更多数据
    }

    void _extract_headers() {
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

    void push_chunk(std::string_view chunk) {
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
            m_body = header.substr(header_len + 4);
            m_header.resize(header_len);
            // 开始分析头部，尝试提取 Content-length 字段
            _extract_headers();
        }
    }

    std::string& headline() {
        return m_headline;
    }

    std::unordered_map<std::string, std::string>& headers() {
        return m_header_keys;
    }

    std::string& headers_raw() {
        return m_header;
    }

    std::string& extra_body() {
        return m_body;
    }
};

template<class HeaderParser = http11_header_parser>
struct _http_base_parser {
    HeaderParser m_header_parser;
    size_t m_content_length = 0;
    size_t body_accumulated_size = 0;
    bool m_body_finished = false;

    void reset_state() {
        m_header_parser.reset_state();
        m_content_length = 0;
        body_accumulated_size = 0;
        m_body_finished = false;
    }

    [[nodiscard]] bool header_finished() {
        return m_header_parser.header_finished();
    }

    [[nodiscard]] bool request_finished() {
        return m_body_finished;
    }

    std::string& headers_raw() {
        return m_header_parser.headers_raw();
    }

    std::string& headline() {
        return m_header_parser.headline();
    }

    std::unordered_map<std::string, std::string>& headers() {
        return m_header_parser.headers();
    }

    std::string _headline_first() {
        // "GET / HTTP/1.1" request
        // "HTTP/1.1 200 OK" response
        auto& line = headline();
        size_t space = line.find(' ');
        if (space == std::string::npos) {
            return "";
        }
        return line.substr(0, space);
    }

    std::string _headline_second() {
        // "GET / HTTP/1.1"
        auto& line = headline();
        size_t space1 = line.find(' ');
        if (space1 == std::string::npos) {
            return "";
        }
        size_t space2 = line.find(' ', space1 + 1);
        if (space2 == std::string::npos) {
            return "";
        }
        return line.substr(space1 + 1, space2 - (space1 + 1));
    }

    std::string _headline_third() {
        // "GET / HTTP/1.1"
        auto& line = headline();
        size_t space1 = line.find(' ');
        if (space1 == std::string::npos) {
            return "";
        }
        size_t space2 = line.find(' ', space1 + 1);
        if (space2 == std::string::npos) {
            return "";
        }
        return line.substr(space2);
    }

    std::string& body() {
        return m_header_parser.extra_body();
    }

    size_t _extract_content_length() {
        auto& headers = m_header_parser.headers();
        auto it = headers.find("content-length");
        if (it == headers.end()) {
            return 0;
        }
        try {
            return std::stoi(it->second);
        } catch (std::logic_error const&) {
            return 0;
        }
    }

    void push_chunk(std::string_view chunk) {
        assert(!m_body_finished);
        if (!m_header_parser.header_finished()) {
            m_header_parser.push_chunk(chunk);
            if (m_header_parser.header_finished()) {
                body_accumulated_size = body().size();
                m_content_length = _extract_content_length();
                if (body_accumulated_size >= m_content_length) {
                    m_body_finished = true;
                }
            }
        } else {
            body().append(chunk);
            body_accumulated_size += chunk.size();
            if (body_accumulated_size >= m_content_length) {
                m_body_finished = true;
            }
        }
    }

    std::string read_some_body() {
        return std::move(body());
    }
};

template<class HeaderParser = http11_header_parser>
struct http_request_parser: _http_base_parser<HeaderParser> {
    HttpMethod method() {
        return utils::parse_enum<HttpMethod>(this->_headline_first());
    }

    std::string url() {
        return this->_headline_second();
    }

    std::string version() {
        return this->_headline_third();
    }
};

template<class HeaderParser = http11_header_parser>
struct http_response_parser: _http_base_parser<HeaderParser> {
    int status() {
        auto s = this->_headline_second();
        try {
            return std::stoi(s);
        } catch (std::logic_error const&) {
            return -1;
        }
    }

    std::string version() {
        return this->_headline_first();
    }
};

struct http11_header_writer {
    std::string m_buffer;

    void reset_state() {
        m_buffer.clear();
    }

    std::string& buffer() {
        return m_buffer;
    }

    void begin_header(std::string_view first, std::string_view second, std::string_view third) {
        m_buffer.append(first);
        m_buffer.append(" ");
        m_buffer.append(second);
        m_buffer.append(" ");
        m_buffer.append(third);
    }

    void write_header(std::string_view key, std::string_view value) {
        m_buffer.append("\r\n");
        m_buffer.append(key);
        m_buffer.append(": ");
        m_buffer.append(value);
    }

    void end_header() {
        m_buffer.append("\r\n\r\n");
    }
};

template<class HeaderWriter = http11_header_writer>
struct _http_base_writer {
    HeaderWriter m_header_writer;

    void _begin_header(std::string_view first, std::string_view second, std::string_view third) {
        m_header_writer.begin_header(first, second, third);
    }

    void reset_state() {
        m_header_writer.reset_state();
    }

    std::string& buffer() {
        return m_header_writer.buffer();
    }

    void write_header(std::string_view key, std::string_view value) {
        m_header_writer.write_header(key, value);
    }

    void end_header() {
        m_header_writer.end_header();
    }

    void write_body(std::string_view body) {
        m_header_writer.buffer().append(body);
    }
};

template<class HeaderWriter = http11_header_writer>
struct http_request_writer: _http_base_writer<HeaderWriter> {
    void begin_header(std::string_view method, std::string_view url) {
        this->_begin_header(method, url, "HTTP/1.1");
    }
};

template<class HeaderWriter = http11_header_writer>
struct http_response_writer: _http_base_writer<HeaderWriter> {
    void begin_header(int status) {
        this->_begin_header("HTTP/1.1", std::to_string(status), "OK");
    }
};

class HttpParser: public BaseParser<HttpRequest, HttpResponse> {
public:
    HttpParser() = default;

    std::vector<uint8_t> write_req(const HttpRequest& req) override {
        m_req_writer.reset_state();
        m_req_writer.begin_header(utils::dump_enum(req.method), req.url);
        for (auto& [key, value]: req.headers) {
            m_req_writer.write_header(key, value);
        }
        m_req_writer.end_header();
        m_req_writer.write_body(req.body);
        return std::vector<uint8_t>(m_req_writer.buffer().begin(), m_req_writer.buffer().end());
    }

    std::vector<uint8_t> write_res(const HttpResponse& res) override {
        m_res_writer.reset_state();
        m_res_writer.begin_header(static_cast<int>(res.status_code));
        for (auto& [key, value]: res.headers) {
            m_res_writer.write_header(key, value);
        }
        m_res_writer.end_header();
        m_res_writer.write_body(res.body);
        return std::vector<uint8_t>(m_res_writer.buffer().begin(), m_res_writer.buffer().end());
    }

    void read_req(std::vector<uint8_t>& req, HttpRequest& out_req) override {
        m_req_parser.push_chunk(std::string_view(reinterpret_cast<char*>(req.data()), req.size()));
        if (m_req_parser.request_finished()) {
            out_req.method = m_req_parser.method();
            out_req.url = m_req_parser.url();
            out_req.version = m_req_parser.version();
            out_req.headers = m_req_parser.headers();
            out_req.body = std::move(m_req_parser.body());
            m_req_parser.reset_state();
        }
    }

    void read_res(std::vector<uint8_t>& res, HttpResponse& out_res) override {
        m_res_parser.push_chunk(std::string_view(reinterpret_cast<char*>(res.data()), res.size()));
        if (m_res_parser.request_finished()) {
            out_res.version = m_res_parser.version();
            out_res.status_code = static_cast<HttpResponseCode>(m_res_parser.status());
            out_res.reason = utils::dump_enum(out_res.status_code);
            out_res.headers = m_res_parser.headers();
            out_res.body = std::move(m_res_parser.body());
            m_res_parser.reset_state();
        }
    }

    bool req_read_finished() override {
        return m_req_parser.request_finished();
    }

    bool res_read_finished() override {
        return m_res_parser.request_finished();
    }

private:
    http_response_parser<> m_res_parser;
    http_request_parser<> m_req_parser;
    http_request_writer<> m_req_writer;
    http_response_writer<> m_res_writer;
};

} // namespace net