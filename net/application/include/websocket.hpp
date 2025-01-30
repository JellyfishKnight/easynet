#pragma once

#include "connection.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
#include "tcp.hpp"
#include "websocket_utils.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

namespace net {

enum class WebSocketStatus {
    CONNECTED,
    DISCONNECTED,
};

class WebSocketClient: public HttpClient {
public:
    WebSocketClient(std::shared_ptr<TcpClient> client);

    std::optional<std::string> close();

    std::optional<std::string> read(WebSocketFrame& data);

    std::optional<std::string> write(const WebSocketFrame& data);

    std::optional<std::string> upgrade(const HttpRequest& upgrade_req);

    ~WebSocketClient();

private:
    std::shared_ptr<WebSocketParser> m_parser;

    WebSocketStatus m_websocket_status = WebSocketStatus::DISCONNECTED;
};

class WebSocketServer: public HttpServer {
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

    void add_handler(
        std::function<void(WebSocketFrame& res, WebSocketFrame& req, Connection::SharedPtr conn)>
            handler
    );

private:
    std::unordered_map<ConnectionKey, std::shared_ptr<WebSocketParser>> m_parsers;
    std::function<void(WebSocketFrame& res, WebSocketFrame& req, Connection::SharedPtr conn)>
        m_handler;
    WebSocketStatus m_websocket_status = WebSocketStatus::DISCONNECTED;
};

} // namespace net