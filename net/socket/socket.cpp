#include "socket.hpp"
#include "connection.hpp"
#include "logger.hpp"
#include <cstddef>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace net {

SocketClient::SocketClient(const std::string& ip, const std::string& service, SocketType type) {
    m_fd = ::socket(AF_INET, static_cast<__socket_type>(type), 0);
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
    m_socket_type = type;
}

SocketClient::~SocketClient() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

int SocketClient::get_fd() const {
    return m_fd;
}

SocketType SocketClient::type() const {
    return m_socket_type;
}

void SocketClient::set_logger(const utils::LoggerManager::Logger& logger) {
    m_logger = logger;
    m_logger_set = true;
}

std::string SocketClient::get_error_msg() {
    // read error from system cateogry
    return std::format("{}", std::system_category().message(errno));
}

std::optional<std::string> SocketClient::connect() {
    if (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to connect to server: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::CONNECTED;
    return std::nullopt;
}

std::optional<std::string> SocketClient::read(std::vector<uint8_t>& data) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Client is not connected";
    }
    ssize_t num_bytes = ::recv(m_fd, data.data(), data.size(), 0);
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

std::optional<std::string> SocketClient::write(const std::vector<uint8_t>& data) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Client is not connected";
    }
    ssize_t num_bytes = ::send(m_fd, data.data(), data.size(), 0);
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string> SocketClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

SocketServer::SocketServer(const std::string& ip, const std::string& service, SocketType type) {
    m_listen_fd = ::socket(AF_INET, static_cast<__socket_type>(type), 0);
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
    m_socket_type = type;
}

SocketServer::~SocketServer() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

SocketType SocketServer::type() const {
    return m_socket_type;
}

int SocketServer::get_fd() const {
    return m_listen_fd;
}

void SocketServer::set_logger(const utils::LoggerManager::Logger& logger) {
    m_logger = logger;
    m_logger_set = true;
}

std::string SocketServer::get_error_msg() {
    // read error from system cateogry
    return std::format("{}", std::system_category().message(errno));
}

void SocketServer::enable_thread_pool(std::size_t worker_num) {
    m_thread_pool = std::make_shared<utils::ThreadPool>(worker_num);
}

std::optional<std::string> SocketServer::enable_epoll(std::size_t event_num) {
    m_epoll_fd = ::epoll_create1(0);
    if (m_epoll_fd == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to create epoll: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_events.resize(event_num);
    m_epoll_enabled = true;
    return std::nullopt;
}

std::optional<std::string> SocketServer::listen() {
    int opt = 1;
    auto ret = ::setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to set socket options: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (::bind(m_listen_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len)
        == -1)
    {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to bind socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (::listen(m_listen_fd, 10) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to listen on socket: {}", get_error_msg());
        }
        return get_error_msg();
    }

    m_status = ConnectionStatus::LISTENING;
    return std::nullopt;
}

std::optional<std::string> SocketServer::read(std::vector<uint8_t>& data, const Connection& conn) {
    if (m_status != ConnectionStatus::CONNECTED || conn.m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::recv(conn.m_client_fd, data.data(), data.size(), 0);
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

std::optional<std::string>
SocketServer::read(std::vector<uint8_t>& data, const struct ::epoll_event& conn) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::recv(conn.data.fd, data.data(), data.size(), 0);
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

std::optional<std::string>
SocketServer::write(const std::vector<uint8_t>& data, const Connection& conn) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::send(conn.m_client_fd, data.data(), data.size(), 0);
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string>
SocketServer::write(const std::vector<uint8_t>& data, const struct ::epoll_event& conn) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::send(conn.data.fd, data.data(), data.size(), 0);
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string> SocketServer::close() {
    m_stop = true;
    m_accept_thread.join();
    if (m_epoll_enabled) {
        ::close(m_epoll_fd);
        m_epoll_enabled = false;
    }
    for (auto& [Key, Conn]: m_connections) {
        ::close(Conn.m_client_fd);
    }
    m_connections.clear();
    if (m_thread_pool) {
        m_thread_pool->stop();
        m_thread_pool.reset();
    }
    if (::close(m_listen_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<std::string> SocketServer::start() {
    if (m_status != ConnectionStatus::LISTENING) {
        return "Server is not listening";
    }
    if (m_default_handler == nullptr) {
        return "No handler set";
    }
    m_stop = false;
    if (m_epoll_enabled) {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                std::vector<struct ::epoll_event> events(m_events.size());
                int num_events = ::epoll_wait(m_epoll_fd, events.data(), m_events.size(), -1);
                if (num_events == -1) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to wait for events: {}", get_error_msg());
                    }
                    continue;
                }
                for (int i = 0; i < num_events; ++i) {
                    if (events[i].data.fd == m_listen_fd) {
                        addressResolver::address client_addr;
                        socklen_t len;
                        int client_fd = ::accept(m_listen_fd, &client_addr.m_addr, &len);
                        if (client_fd == -1) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to accept Connection: {}",
                                    get_error_msg()
                                );
                            }
                            std::cerr << std::format(
                                "Failed to accept Connection: {}\n",
                                get_error_msg()
                            );
                            continue;
                        }
                        struct ::epoll_event event;
                        event.events = EPOLLIN;
                        event.data.fd = client_fd;
                        if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(
                                    m_logger,
                                    "Failed to add client socket to epoll: {}",
                                    get_error_msg()
                                );
                            }
                            std::cerr << std::format(
                                "Failed to add client socket to epoll: {}\n",
                                get_error_msg()
                            );
                            continue;
                        }
                    } else {
                        if (m_thread_pool) {
                            m_thread_pool->submit([this, &events, i]() {
                                handle_connection_epoll(events[i]);
                            });
                        } else {
                            auto unused_future =
                                std::async(std::launch::async, [this, &events, i]() {
                                    handle_connection_epoll(events[i]);
                                });
                        }
                    }
                }
            }
        });
    } else {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                // use select to avoid blocking on accept
                ::fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(m_listen_fd, &readfds);
                struct timeval timeout = { 1, 0 };
                int ret = ::select(m_listen_fd + 1, &readfds, nullptr, nullptr, &timeout);
                if (ret == -1) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to wait for events: {}", get_error_msg());
                    }
                    std::cerr << std::format("Failed to wait for events: {}\n", get_error_msg());
                    continue;
                }
                if (ret == 0) {
                    continue;
                }
                if (FD_ISSET(m_listen_fd, &readfds)) {
                    addressResolver::address_info client_addr;
                    int client_fd = ::accept(
                        m_listen_fd,
                        client_addr.get_address().m_addr,
                        &client_addr.get_address().m_len
                    );
                    if (client_fd == -1) {
                        if (m_logger_set) {
                            NET_LOG_ERROR(
                                m_logger,
                                "Failed to accept Connection: {}",
                                get_error_msg()
                            );
                        }
                        std::cerr
                            << std::format("Failed to accept Connection: {}\n", get_error_msg());
                        continue;
                    }
                    std::string client_ip = ::inet_ntoa(
                        ((struct sockaddr_in*)client_addr.get_address().m_addr)->sin_addr
                    );
                    std::string client_service = std::to_string(
                        ntohs(((struct sockaddr_in*)client_addr.get_address().m_addr)->sin_port)
                    );
                    auto& conn = m_connections[{ client_ip, client_service }];
                    conn.m_client_fd = client_fd;
                    conn.m_server_fd = m_listen_fd;
                    conn.m_status = ConnectionStatus::CONNECTED;
                    conn.m_server_ip = m_ip;
                    conn.m_server_service = m_service;
                    conn.m_client_ip = client_ip;
                    conn.m_client_service = client_service;
                    conn.m_addr = client_addr;
                    if (m_thread_pool) {
                        m_thread_pool->submit([this, &conn]() { handle_connection(conn); });
                    } else {
                        auto unused_future = std::async(std::launch::async, [this, &conn]() {
                            handle_connection(conn);
                        });
                    }
                }
            }
        });
    }
    m_status = ConnectionStatus::CONNECTED;
    return std::nullopt;
}

void SocketServer::handle_connection(Connection& conn) {
    try {
        while (true) {
            if (m_default_handler == nullptr) {
                throw std::runtime_error("No handler set");
            }
            std::vector<uint8_t> req(1024), res;
            this->read(req, conn);
            m_default_handler(res, req);
            if (res.empty()) {
                continue;
            }
            this->write(res, conn);
        }
    } catch (std::runtime_error const& e) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "handler closed because of exception: {}", e.what());
        }
        std::cerr << std::format("handler closed because of exception: {}\n", e.what());
    }
}

void SocketServer::handle_connection_epoll(const struct ::epoll_event& event) {
    if (m_default_handler == nullptr) {
        throw std::runtime_error("No handler set");
    }
    std::vector<uint8_t> buffer(1024), res;
    this->read(buffer, event);
    m_default_handler(res, buffer);
    if (res.empty()) {
        return;
    }
    this->write(res, event);
}

void SocketServer::add_handler(
    std::function<void(std::vector<uint8_t>&, const std::vector<uint8_t>&)>& handler
) {
    m_default_handler = handler;
}

} // namespace net