#include "http_server.hpp"
#include "enum_parser.hpp"
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

} // namespace net
