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
}




} // namespace net