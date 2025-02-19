#pragma once

#include "defines.hpp"
#include "socket_base.hpp"
namespace net {

class TcpClient: public SocketClient {
public:
    NET_DECLARE_PTRS(TcpClient)

    TcpClient(const std::string& ip, const std::string& service);

    TcpClient(const TcpClient&) = delete;

    TcpClient(TcpClient&&) = delete;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&) = delete;

    ~TcpClient();

    std::optional<NetError> connect(std::size_t time_out = 0) override;

    std::optional<NetError> connect_with_retry(std::size_t time_out, std::size_t retry_time_limit = 0) override;

    std::optional<NetError> close() override;

    std::optional<NetError> read(std::vector<uint8_t>& data, std::size_t time_out = 0) override;

    std::optional<NetError> write(const std::vector<uint8_t>& data, std::size_t time_out = 0) override;
};

class TcpServer: public SocketServer {
public:
    NET_DECLARE_PTRS(TcpServer)

    TcpServer(const std::string& ip, const std::string& service);

    TcpServer(const TcpServer&) = delete;

    TcpServer(TcpServer&&) = delete;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&) = delete;

    ~TcpServer();

    std::optional<NetError> listen() override;

    std::optional<NetError> close() override;

    std::optional<NetError> start() override;

    std::optional<NetError> read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) override;

    std::optional<NetError> write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) override;

protected:
    void handle_connection(RemoteTarget::SharedPtr remote) override;

    RemoteTarget::SharedPtr create_remote(int remote_fd) override;

    void add_remote_event(int fd) override;
};

} // namespace net