#include "websocket.hpp"
#include "enum_parser.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
#include "remote_target.hpp"
#include "websocket_utils.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
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

WebSocketStatus WebSocketClient::ws_status() const {
    return m_websocket_status;
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
        return "Failed to parse frame";
    }
}

std::optional<std::string> WebSocketClient::write_ws(const WebSocketFrame& data) {
    return m_client->write(m_parser->write_frame(data));
}

WebSocketClient::~WebSocketClient() {
    if (m_client) {
        if (m_client->status() == SocketStatus::CONNECTED) {
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
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::get(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_get(path, headers, version);
}

HttpResponse WebSocketClient::post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::post(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_post(path, body, headers, version);
}

HttpResponse WebSocketClient::put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::put(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_put(path, body, headers, version);
}

HttpResponse WebSocketClient::del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::del(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_del(path, headers, version);
}

HttpResponse WebSocketClient::patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::patch(path, body, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_patch(path, body, headers, version);
}

HttpResponse WebSocketClient::head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::head(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_head(path, headers, version);
}

HttpResponse WebSocketClient::options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::options(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_options(path, headers, version);
}

HttpResponse WebSocketClient::connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& header,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::connect(path, header, version);
}

std::future<HttpResponse> WebSocketClient::async_connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_connect(path, headers, version);
}

HttpResponse WebSocketClient::trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::trace(path, headers, version);
}

std::future<HttpResponse> WebSocketClient::async_trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::async_trace(path, headers, version);
}

std::optional<std::string> WebSocketClient::write_http(const HttpRequest& req) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::write_http(req);
}

std::optional<std::string> WebSocketClient::read_http(HttpResponse& res) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    return HttpClient::read_http(res);
}

std::shared_ptr<WebSocketClient> WebSocketClient::get_shared() {
    return std::static_pointer_cast<WebSocketClient>(HttpClient::get_shared());
}

/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */
/*************************WebSocket Server************************ */

WebSocketServer::WebSocketServer(std::shared_ptr<TcpServer> server): HttpServer(server) {
    set_handler();
}

SocketStatus WebSocketServer::status() const {
    return m_server->status();
}

void WebSocketServer::get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::get(path, handler);
}

void WebSocketServer::post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::post(path, handler);
}

void WebSocketServer::put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::put(path, handler);
}

void WebSocketServer::del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::del(path, handler);
}

void WebSocketServer::head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::head(path, handler);
}

void WebSocketServer::trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::trace(path, handler);
}

void WebSocketServer::connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::connect(path, handler);
}

void WebSocketServer::options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::options(path, handler);
}

void WebSocketServer::patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    assert(m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!");
    HttpServer::patch(path, handler);
}

std::optional<std::string> WebSocketServer::accept_ws_connection(const HttpRequest& req, std::vector<uint8_t>& res) {
    if (req.headers().find("sec-websocket-key") == req.headers().end()) {
        return "Invalid websocket request";
    }
    auto key = req.headers().at("sec-websocket-key");
    auto accept_key = net::generate_websocket_accept_key(key);
    HttpResponse response;
    response.set_version(HTTP_VERSION_1_1)
        .set_status_code(HttpResponseCode::SWITCHING_PROTOCOLS)
        .set_reason(std::string(utils::dump_enum(HttpResponseCode::SWITCHING_PROTOCOLS)))
        .set_header("Upgrade", "websocket")
        .set_header("Connection", "Upgrade")
        .set_header("Sec-WebSocket-Accept", accept_key)
        .set_header("Sec-WebSocket-Version", "13");
    HttpParser parser;
    res = parser.write_res(response);
    return std::nullopt;
}

void WebSocketServer::set_handler() {
    auto http_handler = [this](RemoteTarget::SharedPtr remote) {
        std::vector<uint8_t> req(1024);
        std::vector<uint8_t> res;
        auto err = m_server->read(req, remote);
        if (err.has_value()) {
            std::cerr << "Failed to read from socket: " << err.value() << std::endl;
            erase_parser(remote->fd());
            return;
        }

        HttpRequest request;
        HttpResponse response;
        if (!m_parsers.contains(remote->fd())) {
            m_parsers.insert({ remote->fd(), std::make_shared<HttpParser>() });
        }
        auto& parser = m_parsers.at(remote->fd());
        parser->add_req_read_buffer(req);
        while (true) {
            auto req_opt = parser->read_req();
            if (!req_opt.has_value()) {
                break;
            }
            auto& request = req_opt.value();
            HttpResponse response;
            auto method = request.method();
            auto path = request.url();
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>::iterator handler;
            // check if the request is a websocket upgrade request
            if (request.headers().find("upgrade") != request.headers().end() && m_allowed_paths.contains(path)) {
                // check if request is correct
                if (request.headers().at("upgrade") == "websocket" && request.headers().at("connection") == "Upgrade") {
                    {
                        std::lock_guard<std::mutex> lock_guard(m_ws_parsers_mutex);
                        if (!m_ws_parsers.contains(remote->fd())) {
                            m_ws_parsers.insert({ remote->fd(), std::make_shared<WebSocketParser>() });
                        }
                        m_ws_connections_flag.insert(remote->fd());
                    }
                    auto err = accept_ws_connection(request, res);
                    if (err.has_value()) {
                        response.set_version(HTTP_VERSION_1_1)
                            .set_status_code(HttpResponseCode::BAD_REQUEST)
                            .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                            .set_header("Content-Length", "0");
                        res = parser->write_res(response);
                    }
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::BAD_REQUEST)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                        .set_header("Content-Length", "0");
                    res = parser->write_res(response);
                }
                err = m_server->write(res, remote);
                if (err.has_value()) {
                    std::cerr << "Failed to write to socket: " << err.value() << std::endl;
                    erase_parser(remote->fd());
                    return;
                }
                break;
            }

            // handle normal http request
            try {
                handler = m_handlers.at(method).find(path);
            } catch (const std::out_of_range& e) {
                if (m_error_handlers.find(HttpResponseCode::BAD_REQUEST) != m_error_handlers.end()) {
                    response = m_error_handlers.at(HttpResponseCode::BAD_REQUEST)(request);
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::BAD_REQUEST)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                        .set_header("Content-Length", "0");
                }
            }

            if (handler == m_handlers.at(method).end()) {
                if (m_error_handlers.find(HttpResponseCode::NOT_FOUND) != m_error_handlers.end()) {
                    response = m_error_handlers.at(HttpResponseCode::NOT_FOUND)(request);
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::NOT_FOUND)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::NOT_FOUND)))
                        .set_header("Content-Length", "0");
                }
            } else {
                try {
                    response = handler->second(request);
                } catch (const HttpResponseCode& e) {
                    if (m_error_handlers.find(e) != m_error_handlers.end()) {
                        response = m_error_handlers.at(e)(request);
                    } else {
                        response.set_version(HTTP_VERSION_1_1)
                            .set_status_code(e)
                            .set_reason(std::string(utils::dump_enum(e)))
                            .set_header("Content-Length", "0");
                    }
                }
            }
            res = parser->write_res(response);
            err = m_server->write(res, remote);
            if (err.has_value()) {
                std::cerr << "Failed to write to socket: " << err.value() << std::endl;
                erase_parser(remote->fd());
                return;
            }
        };
    };

    auto handler = [this, http_handler](RemoteTarget::SharedPtr remote) {
        // parse request
        if (m_ws_connections_flag.contains(remote->fd())) {
            m_ws_handler(remote);
        } else {
            http_handler(remote);
        }
    };

    m_server->on_start(std::move(handler));
}

void WebSocketServer::erase_parser(int remote_fd) {
    {
        std::lock_guard<std::mutex> lock_guard(m_ws_parsers_mutex);
        if (m_ws_parsers.contains(remote_fd)) {
            m_ws_parsers.erase(remote_fd);
        }
    }
    HttpServer::erase_parser(remote_fd);
}

WebSocketServer::~WebSocketServer() {}

WebSocketStatus WebSocketServer::ws_status() const {
    return m_websocket_status;
}

void WebSocketServer::add_websocket_handler(std::function<void(RemoteTarget::SharedPtr remote)> handler) {
    m_ws_handler = std::move(handler);
}

void WebSocketServer::allowed_path(const std::string& path) {
    m_allowed_paths.insert(path);
}

std::shared_ptr<WebSocketServer> WebSocketServer::get_shared() {
    return std::static_pointer_cast<WebSocketServer>(HttpServer::get_shared());
}

std::optional<std::string>
WebSocketServer::write_websocket_frame(const WebSocketFrame& frame, RemoteTarget::SharedPtr remote) {
    assert(m_ws_connections_flag.contains(remote->fd()) && "RemoteTarget is not a websocket connection");
    std::vector<uint8_t> data;
    auto parser = m_ws_parsers.at(remote->fd());
    data = parser->write_frame(frame);
    return m_server->write(data, remote);
}

std::optional<std::string>
WebSocketServer::read_websocket_frame(WebSocketFrame& frame, RemoteTarget::SharedPtr remote) {
    assert(m_ws_connections_flag.contains(remote->fd()) && "RemoteTarget is not a websocket connection");
    auto parser = m_ws_parsers.at(remote->fd());
    std::vector<uint8_t> data(1024);
    auto err = m_server->read(data, remote);
    if (err.has_value()) {
        return err;
    }
    auto result = parser->read_frame(data);
    if (!result.has_value()) {
        return "Not finished yet";
    }
    frame = std::move(result.value());
    return std::nullopt;
}

} // namespace net