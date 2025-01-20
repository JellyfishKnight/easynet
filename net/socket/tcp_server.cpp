#include "tcp_server.hpp"

namespace net {

void TcpServer::write_res(const std::vector<uint8_t>& res, const Connection& fd) {
    if (::send(fd.m_client_fd, res.data(), res.size(), 0) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
}

void TcpServer::read_req(std::vector<uint8_t>& req, const Connection& fd) {
    std::vector<uint8_t> buffer(1024);
    ssize_t num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to receive data");
    }
    if (num_bytes == 0) {
        throw std::system_error(errno, std::system_category(), "Connection closed");
    }
    req = std::move(buffer);
}

void TcpServer::handle_connection() {
    
}

void TcpServer::handle_connection_epoll() {}

} // namespace net