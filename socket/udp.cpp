#include "udp.hpp"
#include <stdexcept>
#include <string>
#include <cstring>


namespace net {

UdpServer::UdpServer(const std::string& ip, int port) {
    // create socket
    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

UdpServer::UdpServer(UdpServer&& other) {
    m_sockfd = other.m_sockfd;
    m_servaddr = other.m_servaddr;
    m_status = other.m_status;

    other.m_sockfd = -1;
    other.m_status = SocketStatus::CLOSED;
}

UdpServer& UdpServer::operator=(UdpServer&& other) {
    if (this != &other) {
        m_sockfd = other.m_sockfd;
        m_servaddr = other.m_servaddr;
        m_status = other.m_status;

        other.m_sockfd = -1;
        other.m_status = SocketStatus::CLOSED;
    }

    return *this;
}

UdpServer::~UdpServer() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

const SocketStatus& UdpServer::status() const {
    return m_status;
}

const struct sockaddr_in& UdpServer::addr() const {
    return m_servaddr;
}

void UdpServer::change_ip(const std::string& ip) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }
    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }
}

void UdpServer::change_port(int port) {
    if (m_status != SocketStatus::CLOSED) {
        ///TOODF: logging
        return ;
    }
    m_servaddr.sin_port = htons(port);
}

void UdpServer::send(const std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (m_sockfd == -1) {
        m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_sockfd == -1) {
            const std::string error_msg(strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }

    if (sendto(m_sockfd, data.data(), data.size(), 0, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to send data: " + error_msg);
    }
}

void UdpServer::close() {
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

UdpClient::UdpClient(const std::string& ip, int port) {
    // create socket
    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

UdpClient::UdpClient(UdpClient&& other) {
    m_sockfd = other.m_sockfd;
    m_servaddr = other.m_servaddr;
    m_status = other.m_status;

    other.m_sockfd = -1;
    other.m_status = SocketStatus::CLOSED;
}

UdpClient& UdpClient::operator=(UdpClient&& other) {
    if (this != &other) {
        m_sockfd = other.m_sockfd;
        m_servaddr = other.m_servaddr;
        m_status = other.m_status;

        other.m_sockfd = -1;
        other.m_status = SocketStatus::CLOSED;
    }

    return *this;
}

const SocketStatus& UdpClient::status() const {
    return m_status;
}

const struct sockaddr_in& UdpClient::addr() const {
    return m_servaddr;
}

void UdpClient::change_ip(const std::string& ip) {
    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Invalid address: " + error_msg);
    }
}

void UdpClient::change_port(int port) {
    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    m_servaddr.sin_port = htons(port);
}

void UdpClient::send(const std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (m_sockfd == -1) {
        m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_sockfd == -1) {
            const std::string error_msg(strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }

    if (sendto(m_sockfd, data.data(), data.size(), 0, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to send data: " + error_msg);
    }
}

void UdpClient::recv(std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (data.empty()) {
        throw std::runtime_error("Data buffer size need to be greater than 0");
    }

    if (m_sockfd == -1) {
        m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_sockfd == -1) {
            const std::string error_msg(strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }

    auto n = recvfrom(m_sockfd, data.data(), data.size(), 0, nullptr, nullptr);
    if (n == -1) {
        const std::string error_msg(strerror(errno));
        throw std::runtime_error("Failed to receive data: " + error_msg);
    }
}

void UdpClient::close() {
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


} // namespace net