#include "ssl.hpp"
#include "socket.hpp"
#include "tcp.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace net {

SSLContext::SSLContext() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    m_ctx = std::shared_ptr<SSL_CTX>(SSL_CTX_new(SSLv23_client_method()), [](SSL_CTX* ctx) {
        SSL_CTX_free(ctx);
    });
    if (m_ctx == nullptr) {
        throw std::runtime_error("Failed to create SSL context");
    }
}

SSLContext::~SSLContext() {
    ERR_free_strings();
    EVP_cleanup();
}

void SSLContext::set_certificates(const std::string& cert_file, const std::string& key_file) {
    if (SSL_CTX_use_certificate_file(m_ctx.get(), cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load certificate file");
    }
    if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error("Failed to load key file");
    }
    if (!SSL_CTX_check_private_key(m_ctx.get())) {
        throw std::runtime_error("Private key does not match the certificate public key");
    }
}

std::shared_ptr<SSL_CTX> SSLContext::get() {
    return m_ctx;
}

SSLClient::SSLClient(
    std::shared_ptr<SSLContext> ctx,
    const std::string& ip,
    const std::string& service
):
    TcpClient(ip, service),
    m_ctx(std::move(ctx)) {
    m_ssl = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    if (m_ssl == nullptr) {
        throw std::runtime_error("Failed to create SSL object");
    }
    SSL_set_fd(m_ssl.get(), m_fd);
}

std::optional<std::string> SSLClient::connect() {
    if (TcpClient::connect().has_value()) {
        return "Failed to connect to server";
    }
    if (SSL_connect(m_ssl.get()) <= 0) {
        return "Failed to establish SSL connection";
    }
    return std::nullopt;
}

std::optional<std::string> SSLClient::write(const std::vector<uint8_t>& data) {
    if (SSL_write(m_ssl.get(), data.data(), data.size()) <= 0) {
        return "Failed to write data";
    }
    return std::nullopt;
}

std::optional<std::string> SSLClient::read(std::vector<uint8_t>& data) {
    int num_bytes = SSL_read(m_ssl.get(), data.data(), data.size());
    if (num_bytes <= 0) {
        return "Failed to read data";
    }
    data.resize(num_bytes);
    return std::nullopt;
}

std::optional<std::string> SSLClient::close() {
    if (SSL_shutdown(m_ssl.get()) == 0) {
        return "Failed to shutdown SSL connection";
    }
    return TcpClient::close();
}

SSLServer::SSLServer(
    std::shared_ptr<SSLContext> ctx,
    const std::string& ip,
    const std::string& service
):
    TcpServer(ip, service),
    m_ctx(std::move(ctx)) {}

std::optional<std::string> SSLServer::listen() {
    auto opt = TcpServer::listen();
    if (opt.has_value()) {
        return opt.value();
    }
    return std::nullopt;
}

std::optional<std::string> SSLServer::read(std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    auto ssl_conn = std::dynamic_pointer_cast<SSLConnection>(conn);
    int num_bytes = SSL_read(ssl_conn->m_ssl.get(), data.data(), data.size());
    if (num_bytes <= 0) {
        return "Failed to read data";
    }
    data.resize(num_bytes);
    return std::nullopt;
}

std::optional<std::string>
SSLServer::write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) {
    auto ssl_conn = std::dynamic_pointer_cast<SSLConnection>(conn);
    if (SSL_write(ssl_conn->m_ssl.get(), data.data(), data.size()) <= 0) {
        return "Failed to write data";
    }
    return std::nullopt;
}

std::optional<std::string> SSLServer::close() {
    std::for_each(m_connections.begin(), m_connections.end(), [](auto& conn) {
        SSL_shutdown(std::dynamic_pointer_cast<SSLConnection>(conn.second)->m_ssl.get());
    });
    return TcpServer::close();
}

std::optional<std::string> SSLServer::start() {
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
                        auto conn = std::make_shared<SSLConnection>();
                        auto ssl_conn = std::dynamic_pointer_cast<SSLConnection>(conn);
                        ssl_conn->m_client_fd = events[i].data.fd;
                        ssl_conn->m_server_fd = m_listen_fd;
                        ssl_conn->m_status = ConnectionStatus::CONNECTED;
                        ssl_conn->m_server_ip = m_ip;
                        ssl_conn->m_server_service = m_service;
                        ssl_conn->m_ssl =
                            std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) {
                                SSL_free(ssl);
                            });
                        if (ssl_conn->m_ssl == nullptr) {
                            if (m_logger_set) {
                                NET_LOG_ERROR(m_logger, "Failed to create SSL object");
                            }
                            std::cerr << "Failed to create SSL object\n";
                            continue;
                        }
                        auto err = get_peer_info(
                            ssl_conn->m_client_fd,
                            ssl_conn->m_client_ip,
                            ssl_conn->m_client_service,
                            ssl_conn->m_addr
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
                        SSL_set_fd(ssl_conn->m_ssl.get(), client_fd);
                    } else {
                        std::string ip, service;
                        addressResolver::address_info info;
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
                    conn = std::make_shared<SSLConnection>();
                    auto ssl_conn = std::dynamic_pointer_cast<SSLConnection>(conn);
                    ssl_conn->m_client_fd = client_fd;
                    ssl_conn->m_server_fd = m_listen_fd;
                    ssl_conn->m_status = ConnectionStatus::CONNECTED;
                    ssl_conn->m_server_ip = m_ip;
                    ssl_conn->m_server_service = m_service;
                    ssl_conn->m_client_ip = client_ip;
                    ssl_conn->m_client_service = client_service;
                    ssl_conn->m_addr = client_addr;
                    ssl_conn->m_ssl =
                        std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) {
                            SSL_free(ssl);
                        });
                    if (ssl_conn->m_ssl == nullptr) {
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to create SSL object");
                        }
                        std::cerr << "Failed to create SSL object\n";
                        continue;
                    }
                    SSL_set_fd(ssl_conn->m_ssl.get(), client_fd);
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

void SSLServer::handle_connection(Connection::SharedPtr conn) {
    auto ssl_conn = std::dynamic_pointer_cast<SSLConnection>(conn);
    if (SSL_accept(ssl_conn->m_ssl.get()) <= 0) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to establish SSL connection: {}", get_error_msg());
        }
        std::cerr << std::format("Failed to establish SSL connection: {}\n", get_error_msg());
        return;
    }
    TcpServer::handle_connection(conn);
}

} // namespace net