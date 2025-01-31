#include "websocket.hpp"
#include "connection.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
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

WebSocketServer::WebSocketServer(std::shared_ptr<TcpServer> server): HttpServer(server) {}

void WebSocketServer::get(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::get(path, handler);
}

void WebSocketServer::post(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::post(path, handler);
}

void WebSocketServer::put(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::put(path, handler);
}

void WebSocketServer::del(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::del(path, handler);
}

void WebSocketServer::head(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::head(path, handler);
}

void WebSocketServer::trace(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::trace(path, handler);
}

void WebSocketServer::connect(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::connect(path, handler);
}

void WebSocketServer::options(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::options(path, handler);
}

void WebSocketServer::patch(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    assert(
        m_websocket_status != WebSocketStatus::CONNECTED && "Protocol has upgraded to websocket!"
    );
    HttpServer::patch(path, handler);
}

std::optional<std::string>
WebSocketServer::accept_ws_connection(const HttpRequest& req, std::vector<uint8_t>& res) {
    if (req.headers().find("Sec-WebSocket-Key") == req.headers().end()) {
        return "Invalid websocket request";
    }
    auto key = req.headers().at("Sec-WebSocket-Key");
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
    auto http_handler = [this](
                            std::vector<uint8_t>& res,
                            std::vector<uint8_t>& req,
                            Connection::ConstSharedPtr conn
                        ) {
        HttpRequest request;
        HttpResponse response;
        if (!m_parsers.contains({ conn->m_client_ip, conn->m_client_service })) {
            m_parsers[{ conn->m_client_ip, conn->m_client_service }] =
                std::make_shared<HttpParser>();
        }
        auto& parser = m_parsers.at({ conn->m_client_ip, conn->m_client_service });
        parser->read_req(req, request);
        if (!parser->req_read_finished()) {
            res.clear();
            return;
        }
        parser->reset_state();
        auto method = request.method();
        auto path = request.url();
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>::iterator
            handler;

        // check if the request is a websocket upgrade request
        if (request.headers().find("Upgrade") != request.headers().end()
            && m_allowed_paths.contains(path))
        {
            // check if request is correct
            if (request.headers().at("Upgrade") == "websocket"
                && request.headers().at("Connection") == "Upgrade")
            {
                if (!m_ws_parsers.contains({ conn->m_client_ip, conn->m_client_service })) {
                    m_ws_parsers[{ conn->m_client_ip, conn->m_client_service }] =
                        std::make_shared<WebSocketParser>();
                }
                m_ws_connections_flag.insert({ conn->m_client_ip, conn->m_client_service });
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
            return;
        }

        // handle normal http request
        try {
            handler = m_handlers.at(method).find(path);
        } catch (const std::out_of_range& e) {
            if (m_error_handlers.find(HttpResponseCode::BAD_REQUEST) != m_error_handlers.end()) {
                response = m_error_handlers.at(HttpResponseCode::BAD_REQUEST)(request);
                res = parser->write_res(response);
                return;
            } else {
                response.set_version(HTTP_VERSION_1_1)
                    .set_status_code(HttpResponseCode::BAD_REQUEST)
                    .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                    .set_header("Content-Length", "0");
            }
            res = parser->write_res(response);
            return;
        }
        if (handler == m_handlers.at(method).end()) {
            if (m_error_handlers.find(HttpResponseCode::NOT_FOUND) != m_error_handlers.end()) {
                response = m_error_handlers.at(HttpResponseCode::NOT_FOUND)(request);
                res = parser->write_res(response);
                return;
            } else {
                response.set_version(HTTP_VERSION_1_1)
                    .set_status_code(HttpResponseCode::NOT_FOUND)
                    .set_reason(std::string(utils::dump_enum(HttpResponseCode::NOT_FOUND)))
                    .set_header("Content-Length", "0");
            }
            res = parser->write_res(response);
            return;
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
    };

    auto ws_handler = [this](
                          std::vector<uint8_t>& res,
                          std::vector<uint8_t>& req,
                          Connection::ConstSharedPtr conn
                      ) {
        WebSocketFrame frame;
        auto& parser = m_ws_parsers.at({ conn->m_client_ip, conn->m_client_service });
        auto result = parser->read_frame(req);
        if (!result.has_value()) {
            res.clear();
            return;
        }
        frame = std::move(result.value());
        m_ws_handler(frame, frame, conn);
        res = parser->write_frame(frame);
    };

    auto handler = [this, http_handler, ws_handler](
                       std::vector<uint8_t>& res,
                       std::vector<uint8_t>& req,
                       Connection::ConstSharedPtr conn
                   ) {
        // parse request
        if (m_ws_connections_flag.contains({ conn->m_client_ip, conn->m_client_service })) {
            ws_handler(res, req, conn);
        } else {
            http_handler(res, req, conn);
        }
    };
    m_server->add_handler(std::move(handler));
}

WebSocketServer::~WebSocketServer() {}

WebSocketStatus WebSocketServer::ws_status() const {
    return m_websocket_status;
}

void WebSocketServer::add_websocket_handler(
    std::function<void(WebSocketFrame& res, WebSocketFrame& req, Connection::ConstSharedPtr conn)>
        handler
) {
    m_ws_handler = std::move(handler);
}

void WebSocketServer::allowed_path(const std::string& path) {
    m_allowed_paths.insert(path);
}

} // namespace net