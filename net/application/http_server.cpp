#include "http_server.hpp"
#include "enum_parser.hpp"
#include "event_loop.hpp"
#include "http_parser.hpp"
#include "remote_target.hpp"
#include "ssl.hpp"
#include "timer.hpp"
#include <cassert>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>

namespace net {

HttpServer::HttpServer(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx):
    m_handlers {
        { HttpMethod::GET, m_get_handlers },        { HttpMethod::POST, m_post_handlers },
        { HttpMethod::PUT, m_put_handlers },        { HttpMethod::TRACE, m_trace_handler },
        { HttpMethod::DELETE, m_delete_handlers },  { HttpMethod::OPTIONS, m_options_handler },
        { HttpMethod::CONNECT, m_connect_handler }, { HttpMethod::PATCH, m_patch_handler },
        { HttpMethod::HEAD, m_head_handler },
    } {
    if (ctx) {
        m_server = std::make_shared<SSLServer>(ctx, ip, service);
    } else {
        m_server = std::make_shared<TcpServer>(ip, service);
    }
    set_handler();
}

void HttpServer::get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::GET).insert_or_assign(path, handler);
}

void HttpServer::post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::POST).insert_or_assign(path, handler);
}

void HttpServer::put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::PUT).insert_or_assign(path, handler);
}

void HttpServer::del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::DELETE).insert_or_assign(path, handler);
}

void HttpServer::head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::HEAD).insert_or_assign(path, handler);
}

void HttpServer::trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::TRACE).insert_or_assign(path, handler);
}

void HttpServer::connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::CONNECT).insert_or_assign(path, handler);
}

void HttpServer::options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::OPTIONS).insert_or_assign(path, handler);
}

void HttpServer::patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_handlers.at(HttpMethod::PATCH).insert_or_assign(path, handler);
}

std::optional<NetError> HttpServer::listen() {
    return m_server->listen();
}

std::optional<NetError> HttpServer::close() {
    return m_server->close();
}

std::optional<NetError> HttpServer::start() {
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

SocketStatus HttpServer::status() const {
    return m_server->status();
}

void HttpServer::add_error_handler(HttpResponseCode err_code, std::function<HttpResponse(const HttpRequest&)> handler) {
    m_error_handlers[err_code] = handler;
}

void HttpServer::enable_thread_pool(std::size_t worker_num) {
    m_server->enable_thread_pool(worker_num);
}

std::optional<NetError> HttpServer::enable_event_loop(EventLoopType type) {
    return m_server->enable_event_loop(type);
}

void HttpServer::set_logger(const utils::LoggerManager::Logger& logger) {
    m_server->set_logger(logger);
}

void HttpServer::write_http_res(const HttpResponse& res, RemoteTarget::SharedPtr remote) {
    auto& parser = m_parsers.at(remote->fd());
    auto buffer = parser->write_res(res);
    auto err = m_server->write(buffer, remote);
    if (err.has_value()) {
        std::cerr << std::format("Failed to write to socket: {}\n", err.value().msg) << std::endl;
        erase_parser(remote->fd());
    }
}

void HttpServer::set_handler() {
    auto handler_thread_func = [this](RemoteTarget::SharedPtr remote) {
        {
            std::lock_guard<std::mutex> lock_guard(m_parsers_mutex);
            if (!m_parsers.contains(remote->fd())) {
                m_parsers.insert({ remote->fd(), std::make_shared<HttpParser>() });
            }
        }
        auto& parser = m_parsers.at(remote->fd());
        // parse request
        std::vector<uint8_t> req(1024);
        std::vector<uint8_t> res;
        auto err = m_server->read(req, remote);
        if (err.has_value()) {
            std::cerr << std::format("Failed to read from socket: {}\n", err.value().msg) << std::endl;
            erase_parser(remote->fd());
            return;
        }
        std::optional<HttpRequest> req_opt;
        parser->add_req_read_buffer(req);
        while (true) {
            req_opt = parser->read_req();
            if (!req_opt.has_value()) {
                break;
            }
            auto& request = req_opt.value();
            HttpResponse response;
            auto method = request.method();
            auto path = request.url();

            std::function<HttpResponse(const HttpRequest&)> handler;
            // if method is wrong
            if (m_handlers.find(method) == m_handlers.end()) {
                if (m_error_handlers.find(HttpResponseCode::METHOD_NOT_ALLOWED) != m_error_handlers.end()) {
                    response = m_error_handlers.at(HttpResponseCode::METHOD_NOT_ALLOWED)(request);
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::METHOD_NOT_ALLOWED)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::METHOD_NOT_ALLOWED)))
                        .set_header("Content-Length", "0");
                }
                write_http_res(response, remote);
                continue;
            }
            // if method is correct but path is wrong
            if (m_handlers.at(method).find(path) == m_handlers.at(method).end()) {
                if (m_error_handlers.find(HttpResponseCode::NOT_FOUND) != m_error_handlers.end()) {
                    response = m_error_handlers.at(HttpResponseCode::NOT_FOUND)(request);
                } else {
                    response.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::NOT_FOUND)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::NOT_FOUND)))
                        .set_header("Content-Length", "0");
                }
                write_http_res(response, remote);
                continue;
            }
            // if method and path are correct
            handler = m_handlers.at(method).at(path);
            try {
                response = handler(request);

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
            write_http_res(response, remote);
        };
    };
    m_server->on_start(handler_thread_func);
    m_server->on_read(handler_thread_func);
};

HttpServer::~HttpServer() {
    if (m_server) {
        if (m_server->status() == SocketStatus::CONNECTED) {
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

void HttpServer::erase_parser(int remote_fd) {
    std::lock_guard<std::mutex> lock_guard(m_parsers_mutex);
    m_parsers.erase(remote_fd);
}

} // namespace net
