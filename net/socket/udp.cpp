#include "udp.hpp"
#include "address_resolver.hpp"
#include "socket_base.hpp"
#include <cstdint>
#include <future>
#include <optional>
#include <sys/socket.h>
#include <sys/types.h>
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

UdpServer::UdpServer(const std::string& ip, const std::string& service) {
    m_listen_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_listen_fd == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to create socket: {}", get_error_msg());
        }
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }
    m_addr_info = m_addr_resolver.resolve(ip, service);
    m_ip = ip;
    m_service = service;
    m_logger_set = false;
    m_epoll_enabled = false;
    m_thread_pool = nullptr;
    m_default_handler = nullptr;
    m_stop = true;
    m_status = ConnectionStatus::DISCONNECTED;
    m_socket_type = SocketType::UDP;
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

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::read(std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpServer::write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) {
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

std::optional<std::string> UdpServer::start() {
    if (m_default_handler == nullptr) {
        return "No handler set";
    }
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
                        addressResolver::address_info client_addr;
                        std::vector<uint8_t> buffer(1024);
                        ssize_t num_bytes = ::recvfrom(
                            m_listen_fd,
                            buffer.data(),
                            buffer.size(),
                            0,
                            client_addr.get_address().m_addr,
                            &client_addr.get_address().m_len
                        );
                        if (num_bytes == -1) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to read from socket: {}",
                                    get_error_msg()
                                );
                            }
                            std::cerr
                                << std::format("Failed to read from socket: {}\n", get_error_msg());
                            continue;
                        }
                        if (num_bytes == 0) {
                            if (m_logger_set) {
                                NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
                            }
                            std::cerr << "Connection reset by peer while reading\n";
                            continue;
                        }
                        std::vector<uint8_t> res;
                        m_default_handler(res, buffer, nullptr);
                        if (res.empty()) {
                            continue;
                        }
                        num_bytes = ::sendto(
                            m_listen_fd,
                            res.data(),
                            res.size(),
                            0,
                            client_addr.get_address().m_addr,
                            client_addr.get_address().m_len
                        );
                        if (num_bytes == -1) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to write to socket: {}",
                                    get_error_msg()
                                );
                            }
                            std::cerr
                                << std::format("Failed to write to socket: {}\n", get_error_msg());
                            continue;
                        }
                        if (num_bytes == 0) {
                            if (m_logger_set) {
                                NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
                            }
                            std::cerr << "Connection reset by peer while writing\n";
                            continue;
                        }
                    }
                }
            }
        });
    } else {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                // receive is handled by main thread, and compute is handled by thread pool
                addressResolver::address_info client_addr;
                std::vector<uint8_t> buffer(1024);
                ssize_t num_bytes = ::recvfrom(
                    m_listen_fd,
                    buffer.data(),
                    buffer.size(),
                    MSG_PEEK,
                    client_addr.get_address().m_addr,
                    &client_addr.get_address().m_len
                );
                if (num_bytes == -1) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to peek from socket: {}", get_error_msg());
                    }
                    std::cerr << std::format("Failed to peek from socket: {}\n", get_error_msg());
                    continue;
                }
                // handle func of udp server shall only been exeute once
                auto handle_func =
                    [this, &buffer, &client_addr]() {
                        std::vector<uint8_t> res;
                        m_default_handler(res, buffer, nullptr);
                        if (res.empty()) {
                            return;
                        }
                        ssize_t num_bytes = ::sendto(
                            m_listen_fd,
                            res.data(),
                            res.size(),
                            0,
                            client_addr.get_address().m_addr,
                            client_addr.get_address().m_len
                        );
                        m_default_handler(res, buffer, nullptr);
                        if (res.empty()) {
                            return;
                        }
                        num_bytes = ::sendto(
                            m_listen_fd,
                            res.data(),
                            res.size(),
                            0,
                            client_addr.get_address().m_addr,
                            client_addr.get_address().m_len
                        );
                        if (num_bytes == -1) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to write to socket: {}",
                                    get_error_msg()
                                );
                            }
                            std::cerr
                                << std::format("Failed to write to socket: {}\n", get_error_msg());
                            return;
                        }
                        if (num_bytes == 0) {
                            if (m_logger_set) {
                                NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
                            }
                            std::cerr << std::format("Connection reset by peer while writing\n");
                            return;
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

} // namespace net