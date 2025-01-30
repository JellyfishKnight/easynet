#include "websocket.hpp"
#include "http_parser.hpp"
#include "websocket_utils.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace net {

WebSocketClient::WebSocketClient(std::shared_ptr<TcpClient> client) {
    m_http_client = std::make_shared<HttpClient>(ip, service);
    m_parser = std::make_shared<WebSocketParser>();
    m_writer = std::make_shared<websocket_writer>();
}

std::optional<std::string> WebSocketClient::connect_server() {
    assert(m_client && "Http client is not initialized");
    // upgrade to websocket
    auto err = m_http_client->connect_server();
    if (err.has_value()) {
        return err;
    }
    m_http_client->write_req(HttpRequest {
        .method = HttpMethod::GET,
        .url = "/",
        .headers = {
            { "Connection", "Upgrade" },
            { "Upgrade", "websocket" },
            { "Sec-WebSocket-Version", "13" },
            { "Sec-WebSocket-Key", generate_websocket_key()},
        },
    });
    HttpResponse res;
    err = m_http_client->read_res(res);
    if (err.has_value()) {
        return err;
    }
    if (res.status_code != HttpResponseCode::SWITCHING_PROTOCOLS) {
        return "Failed to upgrade to websocket";
    }
    // upgrade successful
    m_tcp_client = m_http_client->convert2tcp();
    m_client.reset();
    return std::nullopt;
}

std::optional<std::string> WebSocketClient::close() {
    return m_client->close();
}

std::optional<std::string> WebSocketClient::read(std::vector<uint8_t>& data) {
    std::vector<uint8_t> buffer(1024);
    auto err = m_client->read(buffer);
    if (err.has_value()) {
        return err;
    }
    auto frame = m_parser->read_frame(data);
    if (frame.has_value()) {
        data = std::vector<uint8_t>(frame.value().payload().begin(), frame.value().payload().end());
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

int WebSocketClient::get_fd() const {
    return m_http_client ? m_http_client->get_fd() : m_tcp_client->get_fd();
}

SocketType WebSocketClient::type() const {
    return m_http_client ? m_http_client->type() : m_tcp_client->type();
}

void WebSocketClient::set_logger(const utils::LoggerManager::Logger& logger) {
    return m_http_client ? m_http_client->set_logger(logger) : m_tcp_client->set_logger(logger);
}

std::string WebSocketClient::get_ip() const {
    return m_http_client ? m_http_client->get_ip() : m_tcp_client->get_ip();
}

std::string WebSocketClient::get_service() const {
    return m_http_client ? m_http_client->get_service() : m_tcp_client->get_service();
}

ConnectionStatus WebSocketClient::status() const {
    return m_http_client ? m_http_client->status() : m_tcp_client->status();
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