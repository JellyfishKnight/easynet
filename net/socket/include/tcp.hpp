#pragma once

#include "defines.hpp"
#include "socket_base.hpp"
namespace net {

class TcpClient: public SocketClient {
public:
    NET_DECLARE_PTRS(TcpClient)

    TcpClient(const std::string& ip, const std::string& service);

    TcpClient(const TcpClient&) = delete;

    TcpClient(TcpClient&&) = default;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&) = default;

    ~TcpClient();

    std::optional<std::string> connect() override;

    std::optional<std::string> close() override;

    std::optional<std::string> read(std::vector<uint8_t>& data) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data) override;

    std::shared_ptr<TcpClient> get_shared();
};

class TcpServer: public SocketServer {
public:
    NET_DECLARE_PTRS(TcpServer)

    TcpServer(const std::string& ip, const std::string& service);

    TcpServer(const TcpServer&) = delete;

    TcpServer(TcpServer&&) = default;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&) = default;

    ~TcpServer();

    std::optional<std::string> listen() override;

    std::optional<std::string> close() override;

    std::optional<std::string> start() override;

    std::shared_ptr<TcpServer> get_shared();

protected:
    std::optional<std::string>
    read(std::vector<uint8_t>& data, Connection::SharedPtr conn) override;

    std::optional<std::string>
    write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) override;

    void handle_connection(Connection::SharedPtr conn) override;
};

} // namespace net