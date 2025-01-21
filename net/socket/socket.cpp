#include "socket.hpp"
// #include "print.hpp"
#include <cstdint>
#include <exception>
#include <vector>

namespace net {

void TcpServer::write_res(const std::vector<uint8_t>& res, const Connection& fd) {
    int num_bytes = ::send(fd.m_client_fd, res.data(), res.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
    if (num_bytes == 0) {
        throw std::system_error(0, std::system_category(), "Connection reset by peer");
    }
}

void TcpServer::read_req(std::vector<uint8_t>& req, const Connection& fd) {
    std::vector<uint8_t> buffer(1024);
    ssize_t num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to receive data");
    }
    if (num_bytes == 0) {
        throw std::system_error(0, std::system_category(), "Connection reset by peer");
    }
    buffer.resize(num_bytes);
    req = std::move(buffer);
}

void TcpServer::handle_connection(const Connection& conn) {
    try {
        while (true) {
            if (m_default_handler == nullptr) {
                throw std::runtime_error("No handler set");
            }
            std::vector<uint8_t> req;
            read_req(req, conn);
            auto req_parsed = m_parser->read_req(req);
            if (m_parser->req_read_finished()) {
                auto res = m_default_handler(req_parsed);
                auto res_buffer = m_parser->write_res(res);
                try {
                    write_res(res_buffer, conn);
                } catch (std::exception const& e) {
                    std::cerr << std::format("Failed to write response: {}", e.what()) << std::endl;
                }
            }
        }
    } catch (std::system_error const& e) {
        std::cerr << std::format("handler closed because of exception: {}", e.what()) << std::endl;
    }
}

void TcpServer::handle_connection_epoll() {}

} // namespace net