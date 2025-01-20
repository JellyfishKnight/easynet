#pragma once

#include "connection.hpp"
#include <cstdint>
#include <sys/types.h>
#include <vector>

namespace net {

class TcpClient: public Client<std::vector<uint8_t>, std::vector<uint8_t>> {
public:
    TcpClient(const std::string& ip, const std::string& service): Client(ip, service) {}

    TcpClient(const TcpClient&) = delete;

    TcpClient(TcpClient&&) = default;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&) = default;

private:
};

} // namespace net