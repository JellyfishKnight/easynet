#pragma once

#include "connection.hpp"
#include "defines.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
#include "tcp.hpp"
#include "websocket_utils.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace net {

enum class WebSocketStatus {
    CONNECTED,
    CONNECTING,
    DISCONNECTED,
};

class WebSocketClient: public HttpClient {
public:
    NET_DECLARE_PTRS(WebSocketClient)

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

    std::shared_ptr<WebSocketClient> get_shared();

    WebSocketStatus ws_status() const;

    ~WebSocketClient();

private:
    std::shared_ptr<WebSocketParser> m_parser;

    WebSocketStatus m_websocket_status = WebSocketStatus::DISCONNECTED;
};

class WebSocketServer: public HttpServer {
public:
    NET_DECLARE_PTRS(WebSocketServer)

    WebSocketServer(std::shared_ptr<TcpServer> client);

    ~WebSocketServer();

    WebSocketStatus ws_status() const;

    void get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void add_websocket_handler(std::function<void(Connection::ConstSharedPtr conn)> handler);

    void allowed_path(const std::string& path);

    std::shared_ptr<WebSocketServer> get_shared();

    std::optional<std::string>
    write_websocket_frame(const WebSocketFrame& frame, Connection::ConstSharedPtr conn = nullptr);

    std::optional<std::string> read_websocket_frame(WebSocketFrame& frame, Connection::ConstSharedPtr conn = nullptr);

private:
    std::optional<std::string> accept_ws_connection(const HttpRequest& req, std::vector<uint8_t>& res);

    void set_handler() override;

    std::unordered_set<ConnectionKey> m_ws_connections_flag;
    std::unordered_set<std::string> m_allowed_paths;
    std::unordered_map<ConnectionKey, std::shared_ptr<WebSocketParser>> m_ws_parsers;

    std::function<void(Connection::ConstSharedPtr conn)> m_ws_handler;
    WebSocketStatus m_websocket_status = WebSocketStatus::DISCONNECTED;
};

} // namespace net