#include "tcp.hpp"
#include "common.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <utility>

#include "logger.hpp"

namespace net {

TcpClient::TcpClient(const std::string& ip, int port) {
    // create socket
    m_sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(::strerror(errno)));
    }

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address: " + std::string(::strerror(errno)));
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

void TcpClient::connect() {
    if (m_status == SocketStatus::CONNECTED) {
        return;
    }

    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (m_sockfd == -1) {
        m_sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sockfd == -1) {
            throw std::runtime_error("Failed to create socket: " + std::string(::strerror(errno)));
        }
    }

    if (::connect(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        throw std::runtime_error("Failed to connect: " + std::string(::strerror(errno)));
    }

    m_status = SocketStatus::CONNECTED;
}

void TcpClient::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        throw std::runtime_error("Failed to close socket: " + std::string(::strerror(errno)));
    }
    m_sockfd = -1;
    m_status = SocketStatus::CLOSED;
}

int TcpClient::send(const std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    auto n = ::send(m_sockfd, data.data(), data.size(), 0);
    if (n == -1) {
        throw std::runtime_error("Failed to send data: " + std::string(::strerror(errno)));
    }
    return n;
}

int TcpClient::recv(std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (data.empty()) {
        throw std::runtime_error("Data buffer size need to be greater than 0");
    }

    auto n = ::recv(m_sockfd, data.data(), data.size(), 0);
    if (n == -1) {
        throw std::runtime_error("Failed to receive data: " + std::string(::strerror(errno)));
    }

    data.resize(n);
    return n;
}

TcpClient::~TcpClient() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

TcpServer::TcpServer(const std::string& ip, int port) {
    // create socket
    m_sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockfd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(::strerror(errno)));
    }

    assert(!ip.empty());

    // set server address
    m_servaddr.sin_family = AF_INET;
    m_servaddr.sin_port = htons(port);
    if (ip == "localhost") {
        m_servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else if (::inet_pton(AF_INET, ip.c_str(), &m_servaddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address: " + std::string(::strerror(errno)));
    }

    m_status = SocketStatus::CLOSED;

    m_logger = utils::LoggerManager::get_instance().get_logger("TcpServer:" + ip);
}

TcpServer& TcpServer::operator=(TcpServer&& other) {
    if (this != &other) {
        m_sockfd = other.m_sockfd;
        m_servaddr = other.m_servaddr;
        m_client_connections = std::move(other.m_client_connections);
        other.m_sockfd = -1;
        m_status = other.m_status;
    }

    return *this;
}

TcpServer::TcpServer(TcpServer&& other): m_client_callbacks(std::move(other.m_client_callbacks)) {
    m_sockfd = other.m_sockfd;
    m_servaddr = other.m_servaddr;
    m_client_connections = std::move(other.m_client_connections);
    m_status = other.m_status;

    other.m_sockfd = -1;
}

int TcpServer::accept(struct sockaddr_in* client_addr) {
    if (m_status == SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not listening");
    }

    auto client_addr_size = sizeof(*client_addr);

    auto client_sock =
        ::accept(m_sockfd, (struct sockaddr*)(client_addr), (socklen_t*)&client_addr_size);
    if (client_sock == -1) {
        // get error message
        throw std::runtime_error("Failed to accept connection: " + std::string(::strerror(errno)));
    }

    m_status = SocketStatus::CONNECTED;

    return client_sock;
}

void TcpServer::listen(uint32_t waiting_queue_size) {
    if (m_status == SocketStatus::LISTENING) {
        return;
    }

    if (m_status != SocketStatus::CLOSED) {
        throw std::runtime_error("Socket is not closed");
    }

    if (waiting_queue_size < 1) {
        throw std::runtime_error("waiting_queue_size must be >= 1");
    }

    if (m_sockfd == -1) {
        m_sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sockfd == -1) {
            const std::string error_msg(::strerror(errno));
            throw std::runtime_error("Failed to create socket: " + error_msg);
        }
    }

    if (::bind(m_sockfd, (struct sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1) {
        const std::string error_msg(::strerror(errno));
        throw std::runtime_error("Failed to bind: " + error_msg);
    }

    if (::listen(m_sockfd, waiting_queue_size) == -1) {
        const std::string error_msg(::strerror(errno));
        throw std::runtime_error("Failed to listen: " + error_msg);
    }

    m_status = SocketStatus::LISTENING;
}

int TcpServer::recv(const Connection& conn, std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    if (data.empty()) {
        throw std::runtime_error("Data buffer size need to be greater than 0");
    }

    auto n = ::recv(conn.client_sock, data.data(), data.size(), 0);
    if (n == -1) {
        throw std::runtime_error("Failed to receive data: " + std::string(::strerror(errno)));
    }
    return n;
}

int TcpServer::send(const Connection& conn, const std::vector<uint8_t>& data) {
    if (m_status != SocketStatus::CONNECTED) {
        throw std::runtime_error("Socket is not connected");
    }

    auto n = ::send(conn.client_sock, data.data(), data.size(), 0);
    if (n == -1) {
        throw std::runtime_error("Failed to send data: " + std::string(::strerror(errno)));
    }
    return n;
}

void TcpServer::close() {
    if (m_status == SocketStatus::CLOSED) {
        return;
    }

    if (::close(m_sockfd) == -1) {
        throw std::runtime_error("Failed to close socket: " + std::string(::strerror(errno)));
    }
    m_sockfd = -1;
    for (auto& conn: m_client_connections) {
        if (::close(conn.second.client_sock) == -1) {
            throw std::runtime_error(
                "Failed to close client socket: " + std::string(::strerror(errno))
            );
        }
        conn.second.status = SocketStatus::CLOSED;
        conn.second.client_sock = -1;
        conn.second.server_sock = -1;
    }
    m_status = SocketStatus::CLOSED;
    m_server_thread.join();
}

void TcpServer::add_msg_callback(
    const std::string& ip,
    int port,
    std::function<std::vector<uint8_t>&(const std::vector<uint8_t>&)> callback
) {
    m_client_callbacks.insert({ { ip, port, ConnectionType::TCP }, callback });
}

void TcpServer::start(std::size_t buffer_size) {
    assert(buffer_size > 0);
    m_server_thread = std::thread([this, buffer_size]() {
        std::vector<uint8_t> buffer;
        while (m_status != SocketStatus::CLOSED) {
            Connection connection;
            struct sockaddr_in client;
            connection.client_sock = accept(&client);
            connection.server_sock = m_sockfd;
            connection.client_ip = inet_ntoa(client.sin_addr);
            connection.client_port = ntohs(client.sin_port);
            connection.type = ConnectionType::TCP;
            connection.status = SocketStatus::CONNECTED;

            m_client_connections.insert(
                { { connection.client_ip, connection.client_port, ConnectionType::TCP },
                  connection }
            );
            m_client_threads.emplace_back([this, connection, buffer_size]() {
                std::vector<uint8_t> buffer(buffer_size);
                while (this->m_status == SocketStatus::CONNECTED) {
                    buffer.resize(buffer_size);
                    int n;
                    try {
                        n = recv(connection, buffer);
                    } catch (std::runtime_error& e) {
                        NET_LOG_ERROR(
                            m_logger,
                            "Failed to receive data from client {}:{} : {}",
                            connection.client_ip,
                            connection.client_port,
                            e.what()
                        );
                        m_client_connections.erase(
                            { connection.client_ip, connection.client_port, connection.type }
                        );
                        break;
                    }
                    if (n == 0) {
                        NET_LOG_ERROR(
                            m_logger,
                            "Connection from client {}:{} is reset by peer.",
                            connection.client_ip,
                            connection.client_port
                        );
                        m_client_connections.erase(
                            { connection.client_ip, connection.client_port, connection.type }
                        );
                        break;
                    }
                    std::vector<uint8_t> data(buffer.begin(), buffer.begin() + n);
                    if (m_client_callbacks.contains(connection)) {
                        auto response = m_client_callbacks.at(connection)(data);
                        this->send(connection, response);
                    } else {
                        NET_LOG_ERROR(m_logger, "No callback for the request");
                        m_client_connections.erase(
                            { connection.client_ip, connection.client_port, connection.type }
                        );
                        break;
                    }
                }
            });
        }
    });
}

TcpServer::~TcpServer() {
    if (m_status != SocketStatus::CLOSED) {
        close();
    }
}

} // namespace net