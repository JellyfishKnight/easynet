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

    std::optional<std::string> connect(std::size_t time_out = 0) override;

    std::optional<std::string> connect_with_retry(std::size_t time_out, std::size_t retry_time_limit = 0) override;

    std::optional<std::string> close() override;

    std::optional<std::string> read(std::vector<uint8_t>& data, std::size_t time_out = 0) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data, std::size_t time_out = 0) override;

    std::shared_ptr<TcpClient> get_shared();
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

    std::optional<std::string> listen() override;

    std::optional<std::string> close() override;

    std::optional<std::string> start() override;

    std::shared_ptr<TcpServer> get_shared();

    std::optional<std::string> read(std::vector<uint8_t>& data, const RemoteTarget& remote) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data, const RemoteTarget& remote) override;

protected:
    void handle_connection(const RemoteTarget& remote) override;

    RemoteTarget create_remote(int remote_fd) override;

    void erase_remote() override;

    void add_remote_event(int fd) override;
};

} // namespace net