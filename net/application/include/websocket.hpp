#pragma once

#include "connection.hpp"
#include "tcp.hpp"
#include "websocket_utils.hpp"
#include <memory>
#include <unordered_map>

namespace net {

enum class WebSocketStatus {
    CONNECTED,
    DISCONNECTED,
};

class WebSocketClient {
public:
    WebSocketClient(std::shared_ptr<TcpClient> client);

    std::optional<std::string> connect_server(std::string path = "/");

    std::optional<std::string> close();

    std::optional<std::string> read(const WebSocketFrame& data);

    std::optional<std::string> write(const WebSocketFrame& data);

    int get_fd() const;

    SocketType type() const;

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

    ~WebSocketClient();

private:
    std::shared_ptr<TcpClient> m_client;
    std::shared_ptr<WebSocketParser> m_parser;

    WebSocketStatus m_status;
};

class WebSocketServer {
public:
    WebSocketServer(std::shared_ptr<TcpServer> client);

    std::optional<std::string> listen();

    std::optional<std::string> start();

    std::optional<std::string> close();

    std::optional<std::string> read(std::vector<uint8_t>& data, Connection::SharedPtr conn);

    std::optional<std::string> write(const std::vector<uint8_t>& data, Connection::SharedPtr conn);

    ~WebSocketServer();

    void enable_thread_pool(std::size_t worker_num);

    std::optional<std::string> enable_epoll(std::size_t event_num);

    void set_logger(const utils::LoggerManager::Logger& logger);

    int get_fd() const;

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

private:
    std::shared_ptr<TcpServer> m_server;

    std::unordered_map<ConnectionKey, std::shared_ptr<WebSocketParser>> m_parsers;

    WebSocketStatus m_status;
};

} // namespace net