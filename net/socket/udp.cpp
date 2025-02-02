#include "udp.hpp"
#include "address_resolver.hpp"
#include "connection.hpp"
#include "socket_base.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <sys/socket.h>
#include <sys/types.h>
#include <utility>
#include <vector>

namespace net {

UdpClient::UdpClient(const std::string& ip, const std::string& service) {
    m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to create socket: {}", get_error_msg());
        }
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }
    m_addr_info = m_addr_resolver.resolve(ip, service);
    m_ip = ip;
    m_service = service;
    m_logger_set = false;
    m_status = ConnectionStatus::DISCONNECTED;
    m_socket_type = SocketType::UDP;
}

UdpClient::~UdpClient() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

std::optional<std::string> UdpClient::read(std::vector<uint8_t>& data) {
    assert(data.size() > 0 && "Data buffer is empty");
    ssize_t num_bytes = ::recvfrom(
        m_fd,
        data.data(),
        data.size(),
        0,
        m_addr_info.get_address().m_addr,
        &m_addr_info.get_address().m_len
    );
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while reading";
    }
    data.resize(num_bytes);
    return std::nullopt;
}

std::optional<std::string> UdpClient::write(const std::vector<uint8_t>& data) {
    assert(data.size() > 0 && "Data buffer is empty");
    ssize_t num_bytes = ::sendto(
        m_fd,
        data.data(),
        data.size(),
        0,
        m_addr_info.get_address().m_addr,
        m_addr_info.get_address().m_len
    );
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string> UdpClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpClient::connect() {
    return std::nullopt;
}

std::shared_ptr<UdpClient> UdpClient::get_shared() {
    return std::static_pointer_cast<UdpClient>(SocketClient::get_shared());
}

UdpServer::UdpServer(const std::string& ip, const std::string& service) {
    struct ::addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    m_addr_info = m_addr_resolver.resolve(ip, service, &hints);
    m_listen_fd = m_addr_info.create_socket();
    m_ip = ip;
    m_service = service;
    m_logger_set = false;
    m_epoll_enabled = false;
    m_thread_pool = nullptr;
    m_default_handler = nullptr;
    m_stop = true;
    m_status = ConnectionStatus::DISCONNECTED;
    m_socket_type = SocketType::UDP;
    // bind
    if (::bind(m_listen_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len)
        == -1)
    {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to bind socket: {}", get_error_msg());
        }
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }
}

UdpServer::~UdpServer() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::listen() {
    return std::nullopt;
}

std::optional<std::string> UdpServer::read(std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    addressResolver::address client_addr;
    ssize_t num_bytes = ::recvfrom(
        m_listen_fd,
        data.data(),
        data.size(),
        0,
        &client_addr.m_addr,
        &client_addr.m_addr_len
    );
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while reading";
    }
    if (conn == nullptr) {
        conn = std::make_shared<Connection>();
    }
    conn->m_addr.m_addr = client_addr.m_addr;
    conn->m_addr.m_addr_len = client_addr.m_addr_len;
    conn->m_server_fd = m_listen_fd;
    conn->m_status = ConnectionStatus::CONNECTED;
    conn->m_server_ip = m_ip;
    conn->m_server_service = m_service;
    conn->m_client_ip = ::inet_ntoa(((struct sockaddr_in*)&client_addr.m_addr)->sin_addr);
    conn->m_client_service =
        std::to_string(ntohs(((struct sockaddr_in*)&client_addr.m_addr)->sin_port));
    return std::nullopt;
}

std::optional<std::string>
UdpServer::write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    assert(conn != nullptr && "Connection is nullptr");
    ssize_t num_bytes = ::sendto(
        m_listen_fd,
        data.data(),
        data.size(),
        0,
        &conn->m_addr.m_addr,
        conn->m_addr.m_addr_len
    );
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::read(std::vector<uint8_t>& data, Connection::ConstSharedPtr conn) {
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::write(const std::vector<uint8_t>& data, Connection::ConstSharedPtr conn) {
    return std::nullopt;
}

std::optional<std::string> UdpServer::close() {
    if (::close(m_listen_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::start() {
    assert(m_default_handler != nullptr && "No handler set");
    m_stop = false;
    if (m_epoll_enabled) {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                int num_events = ::epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), -1);
                if (num_events == -1) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to wait for events: {}", get_error_msg());
                    }
                    std::cerr << std::format("Failed to wait for events: {}\n", get_error_msg());
                    continue;
                }
                for (int i = 0; i < num_events; ++i) {
                    if (m_events[i].events & EPOLLIN) {
                        Connection::SharedPtr conn = std::make_shared<Connection>();
                        std::vector<uint8_t> buffer(1024);
                        auto err = this->read(buffer, conn);
                        if (err.has_value()) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to read from socket: {}",
                                    err.value()
                                );
                            }
                            std::cerr
                                << std::format("Failed to read from socket: {}\n", err.value());
                            continue;
                        }
                        m_connections.insert_or_assign(
                            { conn->m_client_ip, conn->m_client_service },
                            conn
                        );

                        auto task = [b = std::move(buffer), conn, this]() {
                            std::vector<uint8_t> res, req = b;
                            m_message_handler(res, req, conn);
                            if (res.empty()) {
                                return;
                            }
                            auto err = this->write(res, conn);
                            if (err.has_value()) {
                                if (m_logger_set) {
                                    NET_LOG_ERROR(
                                        m_logger,
                                        "Failed to write to socket: {}",
                                        err.value()
                                    );
                                }
                                std::cerr
                                    << std::format("Failed to write to socket: {}\n", err.value());
                                m_connections.erase({ conn->m_client_ip, conn->m_client_service });
                            }
                        };

                        if (m_thread_pool) {
                            this->m_thread_pool->submit(task);
                        } else {
                            auto unused_future = std::async(std::launch::async, task);
                        }
                    }
                }
            }
        });
    } else {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                // receive is handled by main thread, and compute is handled by thread pool
                Connection::SharedPtr conn = std::make_shared<Connection>();
                std::vector<uint8_t> buffer(1024);
                auto err = this->read(buffer, conn);
                if (err.has_value()) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", err.value());
                    }
                    std::cerr << std::format("Failed to read from socket: {}\n", err.value());
                    continue;
                }
                m_connections.insert_or_assign({ conn->m_client_ip, conn->m_client_service }, conn);
                // handle func of udp server shall only been exeute once
                auto handle_func = [this, buffer_capture = std::move(buffer), conn]() {
                    std::vector<uint8_t> res, req = buffer_capture;
                    m_message_handler(res, req, conn);
                    if (res.empty()) {
                        return;
                    }
                    auto err = this->write(res, conn);
                    if (err.has_value()) {
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", err.value());
                        }
                        std::cerr << std::format("Failed to write to socket: {}\n", err.value());
                        m_connections.erase({ conn->m_client_ip, conn->m_client_service });
                    }
                };
                if (m_thread_pool) {
                    m_thread_pool->submit(handle_func);
                } else {
                    auto unused_future = std::async(std::launch::async, handle_func);
                }
            }
        });
    }
    return std::nullopt;
}

std::shared_ptr<UdpServer> UdpServer::get_shared() {
    return std::static_pointer_cast<UdpServer>(SocketServer::get_shared());
}

[[deprecated("Udp doesn't need connection, this function will cause no effect")]] void
UdpServer::on_accept(std::function<void(Connection::ConstSharedPtr conn)> handler) {}

void UdpServer::on_message(
    std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&, Connection::ConstSharedPtr)>
        handler
) {
    m_message_handler = std::move(handler);
}

} // namespace net