#include "http_server.hpp"
#include "enum_parser.hpp"
#include <format>
#include <iostream>
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

TcpServer& HttpServer::server() {
    return *m_server;
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



} // namespace net
