#include "websocket.hpp"
#include "connection.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "websocket_utils.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace net {

WebSocketClient::WebSocketClient(std::shared_ptr<TcpClient> client): HttpClient(client) {
    m_parser = std::make_shared<WebSocketParser>();
}

std::optional<std::string> WebSocketClient::upgrade(const HttpRequest& upgrade_req) {
    auto res = HttpClient::get(upgrade_req.url(), upgrade_req.headers());
    if (res.status_code() != HttpResponseCode::SWITCHING_PROTOCOLS) {
        return "Failed to upgrade to websocket";
    } else {
        m_websocket_status = WebSocketStatus::CONNECTED;
        return std::nullopt;
    }
}

std::optional<std::string> WebSocketClient::close() {
    return HttpClient::close();
}

std::optional<std::string> WebSocketClient::read_ws(WebSocketFrame& data) {
    std::vector<uint8_t> buffer(1024);
    auto err = m_client->read(buffer);
    if (err.has_value()) {
        return err;
    }
    auto frame = m_parser->read_frame(buffer);
    if (frame.has_value()) {
        data = std::move(frame.value());
        return std::nullopt;
    } else {
        return "Failed to read frame";
    }
}

std::optional<std::string> WebSocketClient::write_ws(const WebSocketFrame& data) {
    return m_client->write(m_parser->write_frame(data));
}

WebSocketClient::~WebSocketClient() {
    if (m_client) {
        if (m_client->status() == ConnectionStatus::CONNECTED) {
            m_client->close();
        }
        m_client.reset();
    }
}

HttpResponse WebSocketClient::get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::get(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_get(path, headers, version);
}

HttpResponse WebSocketClient::post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::post(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_post(path, body, headers, version);
}

HttpResponse WebSocketClient::put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::put(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_put(path, body, headers, version);
}

HttpResponse WebSocketClient::del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::del(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_del(path, headers, version);
}

HttpResponse WebSocketClient::patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::patch(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_patch(path, body, headers, version);
}

HttpResponse WebSocketClient::head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::head(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_head(path, headers, version);
}

HttpResponse WebSocketClient::options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::options(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_options(path, headers, version);
}

HttpResponse WebSocketClient::connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& header,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::connect(path, header, version);
}

std::future<HttpResponse> WebSocketClient::async_connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_connect(path, headers, version);
}

HttpResponse WebSocketClient::trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::trace(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::async_trace(path, headers, version);
}

std::optional<std::string> WebSocketClient::write_http(const HttpRequest& req) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::write_http(req);
}

std::optional<std::string> WebSocketClient::read_http(HttpResponse& res) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    return HttpClient::read_http(res);
}

/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */

WebSocketServer::WebSocketServer(const std::string& ip, const std::string& service) {
    m_server = std::make_shared<HttpServer>(ip, service);
    m_writer = std::make_shared<websocket_writer>();
}

std::optional<std::string> WebSocketServer::listen() {
    m_server->get("/", [this](const HttpRequest& req) {
        if (req.headers.at("Connection") == "Upgrade" && req.headers.at("Upgrade") == "websocket") {
            return HttpResponse {
                .status_code = HttpResponseCode::SWITCHING_PROTOCOLS,
                .headers = {
                    { "Connection", "Upgrade" },
                    { "Upgrade", "websocket" },
                    { "Sec-WebSocket-Accept", generate_websocket_accept_key(req.headers.at("Sec-WebSocket-Key")) },
                },
            };
        }
        return HttpResponse {
            .status_code = HttpResponseCode::BAD_REQUEST,
        };
    });
    return m_server->listen();
}

std::optional<std::string> WebSocketServer::start() {}

std::optional<std::string> WebSocketServer::close() {}

std::optional<std::string>
WebSocketServer::read(std::vector<uint8_t>& data, Connection::SharedPtr conn) {}

std::optional<std::string>
WebSocketServer::write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) {}

WebSocketServer::~WebSocketServer() {}

void WebSocketServer::enable_thread_pool(std::size_t worker_num) {}

std::optional<std::string> WebSocketServer::enable_epoll(std::size_t event_num) {}

void WebSocketServer::set_logger(const utils::LoggerManager::Logger& logger) {}

int WebSocketServer::get_fd() const {}

std::string WebSocketServer::get_ip() const {}

std::string WebSocketServer::get_service() const {}

ConnectionStatus WebSocketServer::status() const {}

void WebSocketServer::add_ssl_context(std::shared_ptr<SSLContext> ctx) {}

} // namespace net