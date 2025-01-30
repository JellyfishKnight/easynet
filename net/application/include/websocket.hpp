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

    std::optional<std::string> read_ws(WebSocketFrame& data);

    std::optional<std::string> write_ws(const WebSocketFrame& data);

    std::optional<std::string> upgrade(const HttpRequest& upgrade_req);

    virtual HttpResponse
    get(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<HttpResponse> async_get(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse
    put(const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<HttpResponse> async_put(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse
    del(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<HttpResponse> async_del(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& header = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual HttpResponse trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<HttpResponse> async_trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<std::string> write_http(const HttpRequest& req) override;

    virtual std::optional<std::string> read_http(HttpResponse& res) override;

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