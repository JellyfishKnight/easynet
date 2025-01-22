#include "http_server.hpp"
#include "enum_parser.hpp"
#include "parser.hpp"
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace net {

HttpServer::HttpServer(const std::string& ip, const std::string& service):
    Server(ip, service),
    m_handlers {
        { HttpMethod::GET, m_get_handlers },        { HttpMethod::POST, m_post_handlers },
        { HttpMethod::PUT, m_put_handlers },        { HttpMethod::TRACE, m_trace_handler },
        { HttpMethod::DELETE, m_delete_handlers },  { HttpMethod::OPTIONS, m_options_handler },
        { HttpMethod::CONNECT, m_connect_handler }, { HttpMethod::PATCH, m_patch_handler },
        { HttpMethod::HEAD, m_head_handler },
    } {
    m_parser = std::make_shared<HttpParser>();
}

void HttpServer::write_res(const HttpResponse& res, const Connection& fd) {
    auto res_buffer = m_parser->write_res(res);
    int num_bytes = ::send(fd.m_client_fd, res_buffer.data(), res_buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
    if (num_bytes == 0) {
        std::cerr << "Connection reset by peer while writing\n";
    }
}

void HttpServer::read_req(HttpRequest& req, const Connection& fd) {
    std::vector<uint8_t> buffer(1024);
    while (!m_parser->req_read_finished()) {
        buffer.resize(1024);
        auto num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to receive data");
        }
        if (num_bytes == 0) {
            ::close(fd.m_client_fd);
            throw std::runtime_error("Connection reset by peer while reading");
        }
        buffer.resize(num_bytes);
        m_parser->read_req(buffer, req);
    }
    m_parser->reset_state();
}

void HttpServer::handle_connection(const Connection& conn) {
    try {
        while (true) {
            HttpRequest req;
            read_req(req, conn);
            if (!m_handlers.contains(req.method)) {
                std::cerr << std::format("No handler for method: {}", utils::dump_enum(req.method))
                          << std::endl;
                continue;
            }
            if (!m_handlers.at(req.method).contains(req.url)) {
                std::cerr << std::format(
                    "No handler for path: {} in method {}",
                    req.url,
                    utils::dump_enum(req.method)
                ) << std::endl;
                continue;
            }
            auto handler = m_handlers.at(req.method).at(req.url);
            auto res = handler(req);
            write_res(res, conn);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to handle connection: " << e.what() << std::endl;
    }
}

void HttpServer::handle_connection_epoll(const struct ::epoll_event& event) {}

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
