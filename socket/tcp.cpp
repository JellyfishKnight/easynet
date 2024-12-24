#include "tcp.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <cstring>
#include <iostream>

namespace net {

TcpClient::TcpClient(const std::string& ip, int port) {
    // create socket
    m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to create socket: " + error_msg);
    }

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }

    m_status = SocketStatus::CLOSED;
}

TcpClient::TcpClient(TcpClient&& other) {
    m_sockfd = other.m_sockfd;
    m_servaddr = other.m_servaddr;
    m_status = other.m_status;

    other.m_sockfd = -1;
    other.m_status = SocketStatus::CLOSED;
}

TcpClient& TcpClient::operator=(TcpClient&& other) {
    if (this != &other) {
        m_sockfd = other.m_sockfd;
        m_servaddr = other.m_servaddr;
        m_status = other.m_status;

        other.m_sockfd = -1;
        other.m_status = SocketStatus::CLOSED;
    }

    return *this;
}

const SocketStatus& TcpClient::status() const {
    return m_status;
}


const struct sockaddr_in& TcpClient::addr() const {
    return m_servaddr;
}

void TcpClient::change_ip(const std::string &ip) {
    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }
    
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }
}

void TcpClient::change_port(int port) {
    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }
    m_servaddr.sin_port = htons(port);
}

void TcpClient::connect() {
    if (m_status == SocketStatus::CONNECTED) {
        return;
    }

    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (m_sockfd == -1) {
        m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sockfd == -1) {
            const std::string error_msg(strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }
    
    if (::connect(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to connect: " + error_msg);
    }

    m_status = SocketStatus::CONNECTED;
}

void TcpClient::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to close socket: " + error_msg);
    }
    m_sockfd = -1;
    m_status = SocketStatus::CLOSED;
}

void TcpClient::send(const std::vector<uint8_t> &data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (::send(m_sockfd, data.data(), data.size(), 0) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to send data: " + error_msg);
    }
}

void TcpClient::recv(std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (data.empty()) {
        throw std::runtime_error("Data buffer size need to be greater than 0");
    }
    
    auto n = ::recv(m_sockfd, data.data(), data.size(), 0);
    if (n == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to receive data: " + error_msg);
    }

    data.resize(n);
}

TcpClient::~TcpClient() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

TcpServer::TcpServer(const std::string& ip, int port) {
    // create socket
    m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to create socket: " + error_msg);
    }

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }
    
    m_status = SocketStatus::CLOSED;
}

TcpServer& TcpServer::operator=(TcpServer&& other) {
    if (this != &other) {
        m_sockfd = other.m_sockfd;
        m_servaddr = other.m_servaddr;
        m_status = other.m_status;

        other.m_sockfd = -1;
        other.m_status = SocketStatus::CLOSED;
    }

    return *this;
}

TcpServer::TcpServer(TcpServer&& other) {
    m_sockfd = other.m_sockfd;
    m_servaddr = other.m_servaddr;
    m_status = other.m_status;

    other.m_sockfd = -1;
    other.m_status = SocketStatus::CLOSED;
}

const SocketStatus& TcpServer::status() const {
    return m_status;
}

const struct sockaddr_in& TcpServer::addr() const {
    return m_servaddr;
}

void TcpServer::change_ip(const std::string &ip) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }
}

void TcpServer::change_port(int port) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }

    m_servaddr.sin_port = htons(port);
}

void TcpServer::accept(const struct sockaddr_in* const client_addr) {
    if (m_status != SocketStatus::LISTENING) {
        throw std::runtime_error("Socket is not listening");
    }

    auto client_addr_size = sizeof(*client_addr);
    m_sockfd = ::accept(m_sockfd, (struct sockaddr*)(client_addr), (socklen_t*)&client_addr_size);
    if (m_sockfd == -1) {
        // get error message
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to accept connection: " + error_msg);
    }

    m_status = SocketStatus::CONNECTED;
}

void TcpServer::listen(uint32_t waiting_queue_size) {
    if (m_status == SocketStatus::LISTENING) {
        return;
    }

    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (m_sockfd == -1) {
        m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sockfd == -1) {
            const std::string error_msg(strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }
    if (::bind(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to bind: " + error_msg);
    }

    if (::listen(m_sockfd, waiting_queue_size) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to listen: " + error_msg);
    }

    m_status = SocketStatus::LISTENING;
}

void TcpServer::recv(std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (data.empty()) {
        throw std::runtime_error("Data buffer size need to be greater than 0");
    }
    
    auto n = ::recv(m_sockfd, data.data(), data.size(), 0);
    if (n == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to receive data: " + error_msg);
    }
}

void TcpServer::send(const std::vector<uint8_t> &data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (::send(m_sockfd, data.data(), data.size(), 0) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to send data: " + error_msg);
    }
}

void TcpServer::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to close socket: " + error_msg);
    }
    m_sockfd = -1;
    m_status = SocketStatus::CLOSED;
}

TcpServer::~TcpServer() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

} // namespace net