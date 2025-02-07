#pragma once

#include "enum_parser.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace net {

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

#define HTTP_VERSION_1_0 "HTTP/1.0"
#define HTTP_VERSION_1_1 "HTTP/1.1"
#define HTTP_VERSION_2_0 "HTTP/2.0"

struct HttpResponse {
public:
    HttpResponse& set_version(const std::string& version);
    HttpResponse& set_status_code(HttpResponseCode status_code);
    HttpResponse& set_reason(const std::string& reason);
    HttpResponse& set_header(const std::string& key, const std::string& value);
    HttpResponse& set_headers(const std::unordered_map<std::string, std::string>& headers);
    HttpResponse& set_body(const std::string& body);

    [[nodiscard]] const std::string& version() const;
    [[nodiscard]] HttpResponseCode status_code() const;
    [[nodiscard]] const std::string& reason() const;
    [[nodiscard]] const std::string& header(const std::string& key) const;
    [[nodiscard]] const std::unordered_map<std::string, std::string>& headers() const;
    [[nodiscard]] const std::string& body() const;

private:
    std::string m_version;
    HttpResponseCode m_status_code;
    std::string m_reason;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;
};

struct HttpRequest {
public:
    HttpRequest& set_version(const std::string& version);
    HttpRequest& set_url(const std::string& url);
    HttpRequest& set_header(const std::string& key, const std::string& value);
    HttpRequest& set_headers(const std::unordered_map<std::string, std::string>& headers);
    HttpRequest& set_body(const std::string& body);
    HttpRequest& set_method(HttpMethod method);

    [[nodiscard]] HttpMethod method() const;
    [[nodiscard]] const std::string& url() const;
    [[nodiscard]] const std::string& version() const;
    [[nodiscard]] const std::string& header(const std::string& key) const;
    [[nodiscard]] const std::unordered_map<std::string, std::string>& headers() const;
    [[nodiscard]] const std::string& body() const;

private:
    HttpMethod m_method;
    std::string m_url;
    std::string m_version;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_body;
};

struct http11_header_parser {
    std::string m_header; // "GET / HTTP/1.1\nHost: 142857.red\r\nAccept:
        // */*\r\nConnection: close"
    std::string m_headline; // "GET / HTTP/1.1"
    std::unordered_map<std::string, std::string> m_header_keys; // {"Host": "142857.red", "Accept": "*/*",
    // "RemoteTarget: close"}
    std::string m_body; // 不小心超量读取的正文（如果有的话）
    bool m_header_finished = false;

    void reset_state();

    [[nodiscard]] bool header_finished();

    void _extract_headers();

    void push_chunk(std::string& chunk);

    std::string& headline();

    std::unordered_map<std::string, std::string>& headers();

    std::string& headers_raw();

    std::string& extra_body();
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
        return line.substr(space2 + 1);
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
            return std::stoul(it->second);
        } catch (std::logic_error const&) {
            return 0;
        }
    }

    void push_chunk(std::string& chunk) {
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
            auto sub = chunk.substr(
                0,
                m_content_length - body_accumulated_size > chunk.size() ? chunk.size()
                                                                        : m_content_length - body_accumulated_size
            );
            chunk = chunk.substr(sub.size());
            body().append(sub);
            body_accumulated_size += sub.size();
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

    void reset_state();

    std::string& buffer();

    void begin_header(std::string_view first, std::string_view second, std::string_view third);

    void write_header(std::string_view key, std::string_view value);

    void end_header();
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

class HttpParser: public std::enable_shared_from_this<HttpParser> {
public:
    HttpParser() = default;

    std::vector<uint8_t> write_req(const HttpRequest& req);

    std::vector<uint8_t> write_res(const HttpResponse& res);

    std::optional<HttpRequest> read_req();

    void add_req_read_buffer(const std::vector<uint8_t>& buffer);

    std::optional<HttpResponse> read_res();

    void add_res_read_buffer(const std::vector<uint8_t>& buffer);

private:
    http_response_parser<> m_res_parser;
    http_request_parser<> m_req_parser;
    http_request_writer<> m_req_writer;
    http_response_writer<> m_res_writer;

    std::string m_req_read_buffer;
    std::string m_res_read_buffer;
    std::string m_req_write_buffer;
    std::string m_res_write_buffer;
};

} // namespace net