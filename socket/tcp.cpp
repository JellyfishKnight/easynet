#include "tcp.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <sys/socket.h>

namespace net {

TcpClient::TcpClient(const std::string& ip, int port) {
    // create socket
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
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

void TcpClient::change_ip(const std::string &ip) {
    auto last_status = m_status;
    if (m_status != SocketStatus::CLOSED) {
        ///TODO: logging
        this->close();
    }
    
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }

    if (last_status == SocketStatus::CONNECTED) {
        ///TODO: logging
        this->connect();
    }
}

void TcpClient::change_port(int port) {
    auto last_status = m_status;
    if (m_status != SocketStatus::CLOSED) {
        ///TODO: logging
        this->close();
    }

    m_servaddr.sin_port = htons(port);

    if (last_status == SocketStatus::CONNECTED) {
        ///TODO: logging
        this->connect();
    }
}

void TcpClient::connect() {
    if (m_status == SocketStatus::CONNECTED) {
        return;
    }

    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (::connect(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        throw std::runtime_error("Failed to connect");
    }

    m_status = SocketStatus::CONNECTED;
}

void TcpClient::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        throw std::runtime_error("Failed to close socket");
    }

    m_status = SocketStatus::CLOSED;
}

void TcpClient::send(const std::vector<uint8_t> &data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (::send(m_sockfd, data.data(), data.size(), 0) == -1) {
        throw std::runtime_error("Failed to send data");
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
        throw std::runtime_error("Failed to receive data");
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
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
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


void TcpServer::change_ip(const std::string &ip) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }


    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }
}

void TcpServer::change_port(int port) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }

    m_servaddr.sin_port = htons(port);
}

void TcpServer::accept() {
    if (m_status != SocketStatus::LISTENING) {
        throw std::runtime_error("Socket is not listening");
    }

    m_sockfd = ::accept(m_sockfd, (struct sockaddr*)&m_servaddr, (socklen_t*)&m_servaddr);
    if (m_sockfd == -1) {
        throw std::runtime_error("Failed to accept connection");
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

    if (::bind(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        throw std::runtime_error("Failed to bind");
    }

    if (::listen(m_sockfd, waiting_queue_size) == -1) {
        throw std::runtime_error("Failed to listen");
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
        throw std::runtime_error("Failed to receive data");
    }
}

void TcpServer::send(const std::vector<uint8_t> &data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (::send(m_sockfd, data.data(), data.size(), 0) == -1) {
        throw std::runtime_error("Failed to send data");
    }
}

void TcpServer::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        throw std::runtime_error("Failed to close socket");
    }

    m_status = SocketStatus::CLOSED;
}

TcpServer::~TcpServer() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

} // namespace net