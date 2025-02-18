#pragma once

#include "defines.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
#include "remote_target.hpp"
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

    std::optional<NetError> close();

    std::optional<NetError> read_ws(WebSocketFrame& data);

    std::optional<NetError> write_ws(const WebSocketFrame& data);

    std::optional<NetError> upgrade(const HttpRequest& upgrade_req);

    virtual std::optional<NetError>
    get(HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<std::optional<NetError>> async_get(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> post(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_post(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError>
    put(HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<std::optional<NetError>> async_put(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError>
    del(HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1) override;

    virtual std::future<std::optional<NetError>> async_del(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> patch(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_patch(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> head(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_head(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> options(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_options(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> connect(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& header = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_connect(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> trace(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::future<std::optional<NetError>> async_trace(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    ) override;

    virtual std::optional<NetError> write_http(const HttpRequest& req) override;

    virtual std::optional<NetError> read_http(HttpResponse& res) override;

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

    SocketStatus status() const;

    void get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) override;

    void add_websocket_handler(std::function<void(RemoteTarget::SharedPtr remote)> handler);

    void allowed_path(const std::string& path);

    std::shared_ptr<WebSocketServer> get_shared();

    std::optional<NetError> write_websocket_frame(const WebSocketFrame& frame, RemoteTarget::SharedPtr remote);

    std::optional<NetError> read_websocket_frame(WebSocketFrame& frame, RemoteTarget::SharedPtr remote);

private:
    std::optional<NetError> accept_ws_connection(const HttpRequest& req, std::vector<uint8_t>& res);

    void set_handler() override;

    void erase_parser(int remote_fd) override;

    std::unordered_set<int> m_ws_connections_flag;
    std::unordered_set<std::string> m_allowed_paths;
    std::unordered_map<int, std::shared_ptr<WebSocketParser>> m_ws_parsers;
    std::mutex m_ws_parsers_mutex;

    std::function<void(RemoteTarget::SharedPtr remote)> m_ws_handler;
    WebSocketStatus m_websocket_status = WebSocketStatus::DISCONNECTED;
};

} // namespace net