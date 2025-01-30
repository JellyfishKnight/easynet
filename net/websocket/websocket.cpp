#include "websocket.hpp"
#include "http_parser.hpp"
#include "websocket_utils.hpp"
#include <cstdint>
#include <vector>

namespace net {

WebSocketClient::WebSocketClient(const std::string& ip, const std::string& service) {
    m_http_client = std::make_shared<HttpClient>(ip, service);
    m_parser = std::make_shared<WebSocketParser>();
    m_writer = std::make_shared<websocket_writer>();
}

std::optional<std::string> WebSocketClient::connect_server() {
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
    m_http_client.reset();
    return std::nullopt;
}

std::optional<std::string> WebSocketClient::close() {
    return m_tcp_client->close();
}

std::optional<std::string> WebSocketClient::read(std::vector<uint8_t>& data) {
    std::vector<uint8_t> buffer(1024);
    auto err = m_tcp_client->read(buffer);
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
    return m_tcp_client->write(m_parser->write_frame(data));
}

void WebSocketClient::add_ssl_context(std::shared_ptr<SSLContext> ctx) {
    m_http_client->add_ssl_context(ctx);
}

WebSocketClient::~WebSocketClient() {
    if (m_http_client) {
        if (m_http_client->status() == ConnectionStatus::CONNECTED) {
            m_http_client->close();
        }
    }
    if (m_tcp_client) {
        m_tcp_client->close();
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

} // namespace net