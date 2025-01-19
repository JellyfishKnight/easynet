#pragma once

#include "connection.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace net {

class SocketConnect: public BaseConnection<std::vector<uint8_t>, std::vector<uint8_t>> {
public:
    void set_handler(
        std::function<std::vector<uint8_t>(std::vector<uint8_t>)> handler,
        const std::string& index = ""
    ) {
        if (index.empty()) [[unlikely]] {
            m_default_handler = handler;
        }
        m_handlers.insert({ index, handler });
    }

    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> get_handler(std::string index = "") {
        if (index.empty()) [[unlikely]] {
            return m_default_handler;
        }
        return m_handlers.at(index);
    }

private:
    std::unordered_map<std::string, std::function<std::vector<uint8_t>(std::vector<uint8_t>)>>
        m_handlers;
    std::function<std::vector<uint8_t>(std::vector<uint8_t>)> m_default_handler;
};

} // namespace net