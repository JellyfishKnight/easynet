#pragma once

#include "socket.hpp"
namespace net {

class TcpClient: public SocketClient {
public:
    TcpClient(const std::string& ip, const std::string& service);

    TcpClient(const TcpClient&) = delete;

    TcpClient(TcpClient&&) = default;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&) = default;
};

class TcpServer: public SocketServer {
public:
    TcpServer(const std::string& ip, const std::string& service);

    TcpServer(const TcpServer&) = delete;

    TcpServer(TcpServer&&) = default;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&) = default;
};

} // namespace net