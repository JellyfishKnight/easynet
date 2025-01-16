#pragma once

#include <algorithm>
#include <cstddef>
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
private:
    std::vector<uint8_t> m_header;

    std::string m_headline;

    std::unordered_map<std::string, std::string> m_headers_keys;

    std::string m_body;

    bool m_header_finished = false;

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

    void push_chunk(std::vector<uint8_t>& chunk) {
        
    }

};


} // namespace net