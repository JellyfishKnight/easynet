#include "tcp.hpp"
#include "socket_base.hpp"
#include <cassert>
#include <netdb.h>
#include <sys/socket.h>

namespace net {

TcpClient::TcpClient(const std::string& ip, const std::string& service) {
    m_fd = ::socket(AF_INET, ::SOCK_STREAM, 0);
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
    m_socket_type = SocketType::TCP;
}

std::optional<std::string> TcpClient::connect() {
    if (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to connect to server: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::CONNECTED;
    return std::nullopt;
}

std::optional<std::string> TcpClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<std::string> TcpClient::read(std::vector<uint8_t>& data) {
    assert(m_status == ConnectionStatus::CONNECTED && "Client is not connected");
    assert(data.size() > 0 && "Data buffer is empty");
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

std::optional<std::string> TcpClient::write(const std::vector<uint8_t>& data) {
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

TcpClient::~TcpClient() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

TcpServer::TcpServer(const std::string& ip, const std::string& service) {
    struct ::addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    m_addr_info = m_addr_resolver.resolve(ip, service);
    m_listen_fd = m_addr_info.create_socket();

    m_ip = ip;
    m_service = service;
    m_logger_set = false;
    m_epoll_enabled = false;
    m_thread_pool = nullptr;
    m_default_handler = nullptr;
    m_stop = true;
    m_status = ConnectionStatus::DISCONNECTED;
    m_socket_type = SocketType::TCP;
}

TcpServer::~TcpServer() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

std::optional<std::string> TcpServer::listen() {
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

std::optional<std::string> TcpServer::close() {
    m_stop = true;
    m_accept_thread.join();
    if (m_epoll_enabled) {
        ::close(m_epoll_fd);
        m_epoll_enabled = false;
    }
    for (auto& [Key, Conn]: m_connections) {
        ::close(Conn->m_client_fd);
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

std::optional<std::string> TcpServer::start() {
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
                        auto conn = std::make_shared<Connection>();
                        conn->m_client_fd = events[i].data.fd;
                        conn->m_server_fd = m_listen_fd;
                        conn->m_status = ConnectionStatus::CONNECTED;
                        conn->m_server_ip = m_ip;
                        conn->m_server_service = m_service;
                        auto err = get_peer_info(
                            conn->m_client_fd,
                            conn->m_client_ip,
                            conn->m_client_service,
                            conn->m_addr
                        );
                        if (err.has_value()) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(m_logger, "Failed to get peer info: {}", err.value());
                            }
                            std::cerr << std::format("Failed to get peer info: {}\n", err.value());
                            continue;
                        } else {
                            m_connections[{ conn->m_client_ip, conn->m_client_service }] = conn;
                        }
                    } else {
                        std::string ip, service;
                        addressResolver::address info;
                        auto err = get_peer_info(events[i].data.fd, ip, service, info);
                        if (err.has_value()) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(m_logger, "Failed to get peer info: {}", err.value());
                            }
                            std::cerr << std::format("Failed to get peer info: {}\n", err.value());
                            continue;
                        }
                        auto& conn = m_connections.at({ ip, service });
                        if (m_thread_pool) {
                            m_thread_pool->submit([this, conn]() { handle_connection(conn); });
                        } else {
                            auto unused_future = std::async(std::launch::async, [this, conn]() {
                                handle_connection(conn);
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
                    addressResolver::address client_addr;
                    int client_fd =
                        ::accept(m_listen_fd, &client_addr.m_addr, &client_addr.m_addr_len);
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
                    std::string client_ip =
                        ::inet_ntoa(((struct sockaddr_in*)&client_addr.m_addr)->sin_addr);
                    std::string client_service =
                        std::to_string(ntohs(((struct sockaddr_in*)&client_addr.m_addr)->sin_port));
                    auto& conn = m_connections[{ client_ip, client_service }];
                    conn = std::make_shared<Connection>();
                    conn->m_client_fd = client_fd;
                    conn->m_server_fd = m_listen_fd;
                    conn->m_status = ConnectionStatus::CONNECTED;
                    conn->m_server_ip = m_ip;
                    conn->m_server_service = m_service;
                    conn->m_client_ip = client_ip;
                    conn->m_client_service = client_service;
                    conn->m_addr = client_addr;
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

std::optional<std::string> TcpServer::read(std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    if (m_status != ConnectionStatus::CONNECTED || conn->m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::recv(conn->m_client_fd, data.data(), data.size(), 0);
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
TcpServer::write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Server is not connected";
    }
    ssize_t num_bytes = ::send(conn->m_client_fd, data.data(), data.size(), 0);
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

void TcpServer::handle_connection(Connection::SharedPtr conn) {
    assert(m_default_handler != nullptr && "No handler set");
    int ctrl = m_epoll_fd ? 0 : 1;
    do {
        std::vector<uint8_t> req(1024), res;
        auto err = this->read(req, conn);
        if (err.has_value()) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", err.value());
            }
            std::cerr << std::format("Failed to read from socket: {}\n", err.value());
            break;
        }
        m_default_handler(res, req, conn);
        if (res.empty()) {
            continue;
        }
        err = this->write(res, conn);
        if (err.has_value()) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", err.value());
            }
            std::cerr << std::format("Failed to write to socket: {}\n", err.value());
            break;
        }
    } while (ctrl);
}

} // namespace net