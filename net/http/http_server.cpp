#include "http_server.hpp"
#include "connection.hpp"
#include "enum_parser.hpp"
#include "http_parser.hpp"
#include <cassert>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace net {

HttpServer::HttpServer(const std::string& ip, const std::string& service):
    m_handlers {
        { HttpMethod::GET, m_get_handlers },        { HttpMethod::POST, m_post_handlers },
        { HttpMethod::PUT, m_put_handlers },        { HttpMethod::TRACE, m_trace_handler },
        { HttpMethod::DELETE, m_delete_handlers },  { HttpMethod::OPTIONS, m_options_handler },
        { HttpMethod::CONNECT, m_connect_handler }, { HttpMethod::PATCH, m_patch_handler },
        { HttpMethod::HEAD, m_head_handler },
    } {
    m_server = std::make_shared<TcpServer>(ip, service);
    set_handler();
}

void HttpServer::get(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::GET)[path] = handler;
}

void HttpServer::post(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::POST)[path] = handler;
}

void HttpServer::put(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::PUT)[path] = handler;
}

void HttpServer::del(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::DELETE)[path] = handler;
}

void HttpServer::head(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::HEAD)[path] = handler;
}

void HttpServer::trace(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::TRACE)[path] = handler;
}

void HttpServer::connect(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::CONNECT)[path] = handler;
}

void HttpServer::options(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::OPTIONS)[path] = handler;
}

void HttpServer::patch(
    const std::string path,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_handlers.at(HttpMethod::PATCH)[path] = handler;
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

void HttpServer::enable_thread_pool(std::size_t worker_num) {
    m_server->enable_thread_pool(worker_num);
}

std::optional<std::string> HttpServer::enable_epoll(std::size_t event_num) {
    return m_server->enable_epoll(event_num);
}

void HttpServer::set_logger(const utils::LoggerManager::Logger& logger) {
    m_server->set_logger(logger);
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

void HttpServer::add_ssl_context(std::shared_ptr<SSLContext> ctx) {
    assert(
        m_server->status() == ConnectionStatus::DISCONNECTED
        && "SSL context can only be added when server is disconnected"
    );
    m_server = std::make_shared<SSLServer>(ctx, m_server->get_ip(), m_server->get_service());
    set_handler();
}

void HttpServer::add_error_handler(
    HttpResponseCode err_code,
    std::function<HttpResponse(const HttpRequest&)> handler
) {
    m_error_handlers[err_code] = handler;
}

void HttpServer::set_handler() {
    auto handler = [this](
                       std::vector<uint8_t>& res,
                       std::vector<uint8_t>& req,
                       Connection::ConstSharedPtr conn
                   ) {
        HttpRequest request;
        HttpResponse response;
        if (!m_parsers.contains({ conn->m_client_ip, conn->m_client_service })) {
            m_parsers[{ conn->m_client_ip, conn->m_client_service }] = std::make_shared<HttpParser>();
        }
        auto& parser = m_parsers.at({ conn->m_client_ip, conn->m_client_service });
        parser->read_req(req, request);
        if (!parser->req_read_finished()) {
            res.clear();
            return;
        }
        parser->reset_state();
        auto method = request.method;
        auto path = request.url;
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>::iterator
            handler;
        try {
            handler = m_handlers.at(method).find(path);
        } catch (const std::out_of_range& e) {
            if (m_error_handlers.find(HttpResponseCode::BAD_REQUEST) != m_error_handlers.end()) {
                response = m_error_handlers.at(HttpResponseCode::BAD_REQUEST)(request);
                res = parser->write_res(response);
                return;
            } else {
                response.version = HTTP_VERSION_1_1;
                response.status_code = HttpResponseCode::BAD_REQUEST;
                response.reason = utils::dump_enum(HttpResponseCode::BAD_REQUEST);
                response.headers["Content-Length"] = "0";
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
                response.version = HTTP_VERSION_1_1;
                response.status_code = HttpResponseCode::NOT_FOUND;
                response.reason = utils::dump_enum(HttpResponseCode::NOT_FOUND);
                response.headers["Content-Length"] = "0";
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
                    response.version = HTTP_VERSION_1_1;
                    response.status_code = e;
                    response.reason = utils::dump_enum(e);
                    response.headers["Content-Length"] = "0";
                }
            }
        }
        res = parser->write_res(response);
    };
    m_server->add_handler(std::move(handler));
}

} // namespace net
