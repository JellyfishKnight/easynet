#include "http_server.hpp"
#include "connection.hpp"
#include "enum_parser.hpp"
#include "http_parser.hpp"
#include <cassert>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>

namespace net {

HttpServer::HttpServer(std::shared_ptr<TcpServer> server):
    m_handlers {
        { HttpMethod::GET, m_get_handlers },        { HttpMethod::POST, m_post_handlers },
        { HttpMethod::PUT, m_put_handlers },        { HttpMethod::TRACE, m_trace_handler },
        { HttpMethod::DELETE, m_delete_handlers },  { HttpMethod::OPTIONS, m_options_handler },
        { HttpMethod::CONNECT, m_connect_handler }, { HttpMethod::PATCH, m_patch_handler },
        { HttpMethod::HEAD, m_head_handler },
    },
    m_server(std::move(server)) {
    set_handler();
}

void HttpServer::get(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::GET).insert_or_assign(path, handler);
}

void HttpServer::post(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::POST).insert_or_assign(path, handler);
}

void HttpServer::put(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::PUT).insert_or_assign(path, handler);
}

void HttpServer::del(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::DELETE).insert_or_assign(path, handler);
}

void HttpServer::head(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::HEAD).insert_or_assign(path, handler);
}

void HttpServer::trace(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::TRACE).insert_or_assign(path, handler);
}

void HttpServer::connect(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::CONNECT).insert_or_assign(path, handler);
}

void HttpServer::options(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::OPTIONS).insert_or_assign(path, handler);
}

void HttpServer::patch(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::PATCH).insert_or_assign(path, handler);
}

std::optional<std::string> HttpServer::listen() {
    return m_server->listen();
}

std::optional<std::string> HttpServer::close() {
    return m_server->close();
}

std::optional<std::string> HttpServer::start() {
    return m_server->start();
}

int HttpServer::get_fd() const {
    return m_server->get_fd();
}

std::string HttpServer::get_ip() const {
    return m_server->get_ip();
}

std::string HttpServer::get_service() const {
    return m_server->get_service();
}

ConnectionStatus HttpServer::status() const {
    return m_server->status();
}

void HttpServer::add_error_handler(
    HttpResponseCode err_code,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_error_handlers[err_code] = handler;
}

void HttpServer::enable_thread_pool(std::size_t worker_num) {
    m_server->enable_thread_pool(worker_num);
}

std::optional<std::string> HttpServer::enable_epoll(std::size_t event_num) {
    return m_server->enable_epoll(event_num);
}

void HttpServer::set_logger(const utils::LoggerManager::Logger& logger) {
    m_server->set_logger(logger);
}

void HttpServer::set_handler() {
    auto parser_thread = [this](Connection::ConstSharedPtr conn) {
        m_response_buffer_queue.insert_or_assign(
            { conn->m_client_ip, conn->m_client_service },
            std::queue<HttpResponse>()
        );
        m_parsers.insert_or_assign(
            { conn->m_client_ip, conn->m_client_service },
            std::make_shared<HttpParser>()
        );
        while (m_server->status() == ConnectionStatus::CONNECTED) {
            // parse request
            std::vector<uint8_t> req(1024);
            std::vector<uint8_t> res;
            auto err = m_server->read(req, conn);
            if (err.has_value()) {
                std::cerr << "Failed to read from socket: " << err.value() << std::endl;
                break;
            }
            HttpRequest request;
            auto& parser = m_parsers.at({ conn->m_client_ip, conn->m_client_service });
            parser->read_req(request, req);
            if (!parser->req_read_finished()) {
                res.clear();
                continue;
            }
        }
    };

    auto handler = [this](Connection::ConstSharedPtr conn) {
        while (m_server->status() == net::ConnectionStatus::CONNECTED) {
            std::vector<uint8_t> req(1024);
            std::vector<uint8_t> res;
            auto err = m_server->read(req, conn);
            if (err.has_value()) {
                std::cerr << "Failed to read from socket: " << err.value() << std::endl;
                break;
            }
            HttpRequest request;
            HttpResponse response;
            if (!m_parsers.contains({ conn->m_client_ip, conn->m_client_service })) {
                m_parsers[{ conn->m_client_ip, conn->m_client_service }] =
                    std::make_shared<HttpParser>();
            }
            auto& parser = m_parsers.at({ conn->m_client_ip, conn->m_client_service });
            auto not_finished = parser->read_req(request, req);
            if (not_finished.has_value()) {
                res.clear();
                continue;
            }
            auto method = request.method();
            auto path = request.url();
            std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>::
                iterator handler;
            // if method is wrong
            try {
                handler = m_handlers.at(method).find(path);
            } catch (const std::out_of_range& e) {
                if (m_error_handlers.find(HttpResponseCode::BAD_REQUEST) != m_error_handlers.end())
                {
                    response = m_error_handlers.at(HttpResponseCode::BAD_REQUEST)(request);
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::BAD_REQUEST)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                        .set_header("Content-Length", "0");
                }
            }
            // if no handler for the path in this method is not found
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
            // write response to socket
            res = parser->write_res(response);
            err = m_server->write(res, conn);
            if (err.has_value()) {
                std::cerr << "Failed to write to socket: " << err.value() << std::endl;
                break;
            }
        }
    };
    m_server->on_accept(std::move(handler));
}

HttpServer::~HttpServer() {
    if (m_server) {
        if (m_server->status() == ConnectionStatus::CONNECTED) {
            m_server->close();
        }
        m_server.reset();
    }
    m_parsers.clear();
    m_error_handlers.clear();
    m_delete_handlers.clear();
    m_get_handlers.clear();
    m_post_handlers.clear();
    m_put_handlers.clear();
    m_trace_handler.clear();
    m_connect_handler.clear();
    m_options_handler.clear();
    m_patch_handler.clear();
    m_head_handler.clear();
}

std::shared_ptr<TcpServer> HttpServer::convert2tcp() {
    auto server = std::move(m_server);
    return std::move(m_server);
}

std::shared_ptr<HttpServer> HttpServer::get_shared() {
    return shared_from_this();
}

} // namespace net
