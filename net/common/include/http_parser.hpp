#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace net {

enum class http_method {
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

enum class http_response_code {
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

class http11_parser {
protected:
    std::vector<uint8_t> m_header;

    std::string m_headline;

    std::unordered_map<std::string, std::string> m_headers_keys;

    std::string m_body;

    bool m_header_finished = false;
public:
    void reset() {
        m_header.clear();
        m_headline.clear();
        m_headers_keys.clear();
        m_body.clear();
        m_header_finished = false;
    }

    [[nodiscard]] bool header_finished() {
        return m_header_finished;
    }
protected:
    void extract_headers() {
        std::string_view header_view(reinterpret_cast<const char*>(m_header.data()), m_header.size());
        std::size_t pos = header_view.find("\r\n", 0, 2);
        m_headline = std::string(header_view.substr(0, pos));
        while (pos != std::string::npos) {
            pos += 2; // Skip \r\n
            // find pos of next \r\n
            std::size_t next_pos = header_view.find("\r\n", pos, 2);
            std::size_t line_len = std::string::npos;
            if (next_pos != std::string::npos) {
                line_len = next_pos - pos;
            }
            std::string_view line = header_view.substr(pos, line_len);
            std::size_t colon_pos = line.find(": ", 0, 2);
            if (colon_pos != std::string::npos) {
                std::string key = std::string(line.substr(0, colon_pos));
                std::string value = std::string(line.substr(colon_pos + 2));
                for (char &c : key) {
                    if ('A' <= c && c <= 'Z') {
                        c = c - 'A' + 'a';
                    }
                }
                m_headers_keys.insert_or_assign(std::move(key), value);
            }
            pos = next_pos;
        }
    }


public:
    void push_chunk(std::vector<uint8_t>& chunk) {
        assert(m_header_finished == false);
        std::size_t old_size = m_header.size();
        m_header.resize(old_size + chunk.size());
        std::string_view header = std::string_view(reinterpret_cast<const char*>(m_header.data()), old_size + chunk.size());
        // if header is still parsering, try judge if it is finished
        if (old_size < 4) {
            old_size = 4;
        }
        old_size -= 4;
        std::size_t header_len = header.find("\r\n\r\n", old_size, 4);
        if (header_len != std::string::npos) {
            // header finished
            m_header_finished = true;
            // leave the body
            m_body = std::string(header.substr(header_len + 4));
            m_header.resize(header_len + 4);
            // extract headers
            extract_headers();
        }
    }

    std::string &headline() {
        return m_headline;
    }

    std::unordered_map<std::string, std::string>& headers() {
        return m_headers_keys;
    }

    std::vector<uint8_t>& header_raw() {
        return m_header;
    }

    std::string &body() {
        return m_body;
    }
};

template<typename HeaderParser = http11_parser>
class http_base_parser {
protected:
    HeaderParser m_header_parser;

    std::size_t m_content_length = 0;
    std::size_t body_accumulated = 0;
    bool m_body_finished = false;
public:
    void reset_state() {
        m_header_parser.reset();
        m_content_length = 0;
        body_accumulated = 0;
        m_body_finished = false;
    }

    [[nodiscard]] bool header_finished() {
        return m_header_parser.header_finished();
    }

    [[nodiscard]] bool request_finished() {
        return m_header_parser.header_finished() && m_body_finished;
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
protected:
    std::string headline_first() {
        auto &line = headline();
        std::size_t space = line.find(' ');
        if (space == std::string::npos) {
            return "";
        }
        return line.substr(0, space);
    }

    std::string headline_second() {
        auto &line = headline();
        std::size_t space = line.find(' ');
        if (space == std::string::npos) {
            return "";
        }
        std::size_t space2 = line.find(' ', space + 1);
        if (space2 == std::string::npos) {
            return "";
        }
        return line.substr(space + 1, space2 - space - 1);
    }

    std::string headline_third() {
        auto &line = headline();
        std::size_t space1 = line.find(' ');
        if (space1 == std::string::npos) {
            return "";
        }
        std::size_t space2 = line.find(' ', space1 + 1);
        if (space2 == std::string::npos) {
            return "";
        }
        return line.substr(space2 + 1);
    }
public:
    std::string& body() {
        return m_header_parser.body();
    }

protected:
    std::size_t extract_content_length() {
        auto &headers = m_header_parser.headers();
        auto it = headers.find("content-length");
        if (it == headers.end()) {
            return 0;
        }
        return std::stoull(it->second);
    }

public:
    void push_chunk(std::vector<uint8_t>& chunk) {
        assert(m_body_finished == false);
        if (!m_header_parser.header_finished()) {
            m_header_parser.push_chunk(chunk);
            if (m_header_parser.header_finished()) {
                body_accumulated = body().size();
                m_content_length = extract_content_length();
                if (body_accumulated >= m_content_length) {
                    m_body_finished = true;
                }
            }
        } else {
            body().append(chunk);
            body_accumulated += chunk.size();
            if (body_accumulated >= m_content_length) {
                m_body_finished = true;
            }
        }
    }

    std::string read_some_body() {
        return std::move(body());
    }
};

template <typename HeaderParser = http11_parser>
class http_request_parser : public http_base_parser<HeaderParser> {
public:
    http_method method() {
        
    }

    std::string url() {
        return this->headline_second();
    }
};

template <typename HeaderParser = http11_parser>
class http_response_parser : public http_base_parser<HeaderParser> {
public:    
    int status() {
        auto s = this->headline_second();
        try {
            return std::stoi(s);
        } catch (std::logic_error const &) {
            return -1;
        }
    }
};

class http11_header_writer {
    std::vector<uint8_t> m_buffer;

    void reset_state() {
        m_buffer.clear();
    }

    std::vector<uint8_t>& buffer() {
        return m_buffer;
    }

    void begin_header(std::string_view first, std::string_view second,
                        std::string_view third) {
        m_buffer.insert(m_buffer.end(), first.begin(), first.end());
        m_buffer.emplace_back(' ');
        m_buffer.insert(m_buffer.end(), second.begin(), second.end());
        m_buffer.emplace_back(' ');
        m_buffer.insert(m_buffer.end(), third.begin(), third.end());
    }

    void write_header(std::string_view key, std::string_view value) {
        static std::string temp ="\r\n";
        m_buffer.insert(m_buffer.end(), temp.begin(), temp.end());
        m_buffer.insert(m_buffer.end(), key.begin(), key.end());
        m_buffer.emplace_back(':');
        m_buffer.emplace_back(' ');
        m_buffer.insert(m_buffer.end(), value.begin(), value.end());
    }

    void end_header() {
        static std::string temp = "\r\n\r\n";
        m_buffer.insert(m_buffer.end(), temp.begin(), temp.end());
    }
};

template<typename HeaderWriter = http11_header_writer>
class http_base_writer {
protected:
    HeaderWriter m_header_writer;

    void begin_header(std::string_view first, std::string_view second,
                        std::string_view third) {
        m_header_writer.begin_header(first, second, third);
    }

    void reset_state() {
        m_header_writer.reset_state();
    }

    std::vector<uint8_t>& buffer() {
        return m_header_writer.buffer();
    }

    void write_header(std::string_view key, std::string_view value) {
        m_header_writer.write_header(key, value);
    } 

    void end_header() {
        m_header_writer.end_header();
    }

    void write_body(std::string_view body) {
        m_header_writer.buffer().insert(m_header_writer.buffer().end(), body.begin(), body.end());
    }
};


template<typename HeaderWriter = http11_header_writer>
class http_request_writer : public http_base_writer<HeaderWriter> {
public:
    void begin_header(std::string_view method, std::string_view url, std::string_view version) {
        this->begin_header(method, url, version);
    }
};

template<typename HeaderWriter = http11_header_writer>
class http_response_writer : public http_base_writer<HeaderWriter> {
public:
    void begin_header(std::string_view version, std::string_view status, std::string_view reason) {
        this->begin_header(version, status, reason);
    }
};




} // namespace net