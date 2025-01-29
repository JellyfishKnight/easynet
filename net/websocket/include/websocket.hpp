#pragma once

#include "http.hpp"
#include "http_client.hpp"
#include "http_server.hpp"
#include "websocket_utils.hpp"

namespace net {

class WebSocketClient {
public:
    WebSocketClient(const std::string& ip, const std::string& service);

    std::optional<std::string> connect_server();

    std::optional<std::string> close();

    std::optional<std::string> read(std::vector<uint8_t>& data);

    std::optional<std::string> write(const std::vector<uint8_t>& data);

    void add_ssl_context(std::shared_ptr<SSLContext> ctx);

    ~WebSocketClient();

private:
    std::shared_ptr<HttpClient> m_client;
    std::shared_ptr<WebSocketParser> m_parser;
    std::shared_ptr<websocket_writer> m_writer;
};

class WebSocketServer {
public:
    WebSocketServer(const std::string& ip, const std::string& service);

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

    void add_ssl_context(std::shared_ptr<SSLContext> ctx);

private:
    std::shared_ptr<HttpServer> m_server;
    std::shared_ptr<WebSocketParser> m_parser;
    std::shared_ptr<websocket_writer> m_writer;
};

} // namespace net