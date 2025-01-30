#include "websocket.hpp"
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

std::optional<std::string> WebSocketClient::read(WebSocketFrame& data) {
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

std::optional<std::string> WebSocketClient::write(const WebSocketFrame& data) {
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