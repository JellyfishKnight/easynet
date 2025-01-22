#include "http_server.hpp"

namespace net {

void HttpServer::write_res(const HttpResponse& res, const Connection& fd) {
    auto res_buffer = m_parser->write_res(res);
    int num_bytes = ::send(fd.m_client_fd, res_buffer.data(), res_buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
    if (num_bytes == 0) {
        throw std::runtime_error("Connection reset by peer while writing");
    }
}

void HttpServer::read_req(HttpRequest& req, const Connection& fd) {
    std::vector<uint8_t> buffer(1024);
    HttpRequest req_t;
    while (!m_parser->req_read_finished()) {
        buffer.resize(1024);
        auto num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to receive data");
        }
        if (num_bytes == 0) {
            throw std::runtime_error("Connection reset by peer while reading");
        }
        buffer.resize(num_bytes);
        m_parser->read_req(buffer, req);
    }
    req = std::move(req_t);
}

void HttpServer::handle_connection(const Connection& conn) {
    try {
        while (true) {
            if (m_default_handler == nullptr) {
                throw std::runtime_error("No handler set");
            }
            HttpRequest req;
            read_req(req, conn);

            // auto res = m_default_handler(req_parsed);
            // write_res(res, conn);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to handle connection: " << e.what() << std::endl;
    }
}

void HttpServer::handle_connection_epoll() {}

} // namespace net
