#include "socket.hpp"
// #include "print.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace net {

std::vector<uint8_t> TcpClient::read_res() {
    std::vector<uint8_t> buffer(1024);
    ssize_t num_bytes = ::recv(m_fd, buffer.data(), buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to receive data");
    }
    if (num_bytes == 0) {
        throw std::runtime_error("Connection reset by peer while reading");
    }
    buffer.resize(num_bytes);
    return buffer;
}

void TcpClient::write_req(const std::vector<uint8_t>& req) {
    if (::send(m_fd, req.data(), req.size(), 0) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
}

void TcpServer::write_res(const std::vector<uint8_t>& res, const Connection& fd) {
    int num_bytes = ::send(fd.m_client_fd, res.data(), res.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
    if (num_bytes == 0) {
        std::cerr << "Connection reset by peer while writing\n";
    }
}

void TcpServer::read_req(std::vector<uint8_t>& req, const Connection& fd) {
    std::vector<uint8_t> buffer(1024);
    ssize_t num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
    if (num_bytes == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to receive data");
    }
    if (num_bytes == 0) {
        ::close(fd.m_client_fd);
        throw std::runtime_error("Connection reset by peer while reading");
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
            auto res = m_default_handler(req);
            write_res(res, conn);
        }
    } catch (std::runtime_error const& e) {
        std::cerr << std::format("handler closed because of exception: {}", e.what()) << std::endl;
    }
}

void TcpServer::handle_connection_epoll(const struct ::epoll_event& event) {
    if (m_default_handler == nullptr) {
        throw std::runtime_error("No handler set");
    }
    std::vector<uint8_t> buffer(1024);
    ssize_t bytes_read = ::read(event.data.fd, buffer.data(), buffer.size());
    if (bytes_read > 0) {
        buffer.resize(bytes_read);
        auto res = m_default_handler(buffer);
        auto bytes_sent = ::send(event.data.fd, res.data(), res.size(), 0);
        if (bytes_sent == -1) {
            ::close(event.data.fd);
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
        if (bytes_sent == 0) {
            ::close(event.data.fd);
            std::cerr << "Connection reset by peer while reading\n";
        }
    } else if (bytes_read == 0) {
        ::close(event.data.fd);
        std::cerr << "Connection reset by peer while reading\n" << std::endl;
    } else {
        ::close(event.data.fd);
        throw std::system_error(errno, std::system_category(), "Failed to read data");
    }
}

} // namespace net