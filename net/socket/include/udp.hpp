#pragma once

#include "socket.hpp"

namespace net {

class UdpClient: public SocketClient {
public:
    UdpClient(const std::string& ip, const std::string& service, SocketType type);

    UdpClient(const UdpClient&) = delete;

    UdpClient(UdpClient&&) = default;

    UdpClient& operator=(const UdpClient&) = delete;

    UdpClient& operator=(UdpClient&&) = default;

    std::optional<std::string> read(std::vector<uint8_t>& data) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data) override;

    std::optional<std::string> close() override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    connect() override;
};

class UdpServer : public SocketServer {
public:
    UdpServer(const std::string& ip, const std::string& service, SocketType type);

    UdpServer(const UdpServer&) = delete;

    UdpServer(UdpServer&&) = default;

    UdpServer& operator=(const UdpServer&) = delete;

    UdpServer& operator=(UdpServer&&) = default;

    std::optional<std::string> listen() override;

    std::optional<std::string> read(std::vector<uint8_t>& data, const Connection& conn) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data, const Connection& conn) override;

    std::optional<std::string> close(const Connection& conn) override;

    std::optional<std::string> close() override;

    


};

}; // namespace net