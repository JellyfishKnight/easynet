#include "tcp.hpp"
#include "defines.hpp"
#include "event_loop.hpp"
#include "logger.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
#include "timer.hpp"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <optional>
#include <ratio>
#include <shared_mutex>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <utility>
#include <vector>

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
    m_status = SocketStatus::DISCONNECTED;
    m_socket_type = SocketType::TCP;
    set_non_blocking_socket(m_fd);
}

std::optional<std::string> TcpClient::connect(std::size_t time_out) {
    if (time_out == 0) {
        while (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
            if (errno == EALREADY || errno == EINPROGRESS) {
                continue;
            } else {
                return get_error_msg();
            }
        }
        m_status = SocketStatus::CONNECTED;
        return std::nullopt;
    }
    Timer timer;
    timer.set_timeout(std::chrono::milliseconds(time_out));
    timer.async_start_timing();
    while (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
        if (errno != EALREADY && errno != EINPROGRESS) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to connect to socket: {}", get_error_msg());
            }
            return get_error_msg();
        }
        if (timer.timeout()) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Timeout to connect to socket, last error: {}", get_error_msg());
            }
            return std::format("Timeout to connect to socket, last error: {}", get_error_msg());
        }
    }
    m_status = SocketStatus::CONNECTED;
    return std::nullopt;
}

std::optional<std::string> TcpClient::connect_with_retry(std::size_t time_out, std::size_t retry_time_limit) {
    std::size_t tried_time = 0;
    while (true) {
        if (tried_time++ >= retry_time_limit) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to connect to socket: {}", get_error_msg());
            }
            return get_error_msg();
        }
        auto ret = this->connect(time_out);
        if (!ret.has_value()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::string> TcpClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = SocketStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<std::string> TcpClient::read(std::vector<uint8_t>& data, std::size_t time_out) {
    assert(m_status == SocketStatus::CONNECTED && "Client is not connected");
    data.clear();
    ssize_t num_bytes;
    Timer timer;
    timer.reset();
    if (time_out != 0) {
        timer.set_timeout(std::chrono::milliseconds(time_out));
        timer.async_start_timing();
    }
    while (true) {
        if (timer.timeout()) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Timeout to read from socket");
            }
            return "Timeout to read from socket";
        }
        std::vector<uint8_t> buffer(1024);
        num_bytes = ::recv(m_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (data.size() <= 0) {
                    continue;
                }
                break;
            }
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
        buffer.resize(num_bytes);
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(data));
    };
    return std::nullopt;
}

std::optional<std::string> TcpClient::write(const std::vector<uint8_t>& data, std::size_t time_out) {
    assert(m_status == SocketStatus::CONNECTED && "Client is not connected");
    assert(data.size() > 0 && "Data buffer is empty");
    size_t bytes_has_send = 0;
    Timer timer;
    timer.reset();
    if (time_out != 0) {
        timer.set_timeout(std::chrono::milliseconds(time_out));
        timer.async_start_timing();
    }
    while (true) {
        if (timer.timeout() && bytes_has_send <= 0) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Timeout to write to socket");
            }
            return "Timeout to write to socket";
        }
        ssize_t num_bytes = ::send(m_fd, data.data() + bytes_has_send, data.size() - bytes_has_send, 0);
        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (bytes_has_send <= 0) {
                    continue;
                }
                if (bytes_has_send != data.size()) {
                    if (m_logger_set) {
                        NET_LOG_WARN(m_logger, "early end of socket");
                    }
                    return "early end of socket";
                }
            }
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
        bytes_has_send += num_bytes;
        if (bytes_has_send >= data.size()) {
            return std::nullopt;
        }
    };
    return std::nullopt;
}

std::shared_ptr<TcpClient> TcpClient::get_shared() {
    return std::static_pointer_cast<TcpClient>(SocketClient::get_shared());
}

TcpClient::~TcpClient() {
    if (m_status == SocketStatus::CONNECTED) {
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
    m_thread_pool = nullptr;
    m_accept_handler = nullptr;
    m_stop = true;
    m_status = SocketStatus::DISCONNECTED;
    m_socket_type = SocketType::TCP;
}

TcpServer::~TcpServer() {
    if (m_status == SocketStatus::CONNECTED) {
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
    if (::bind(m_listen_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
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

    m_status = SocketStatus::LISTENING;
    return std::nullopt;
}

std::optional<std::string> TcpServer::close() {
    m_stop = true;
    m_accept_thread.join();
    m_event_loop.reset();
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
    m_status = SocketStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<std::string> TcpServer::start() {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(m_accept_handler != nullptr && "No handler set");
    m_stop = false;
    if (m_event_loop) {
        m_accept_thread = std::thread([this]() {
            auto err = set_non_blocking_socket(m_listen_fd);
            if (err.has_value()) {
                if (m_logger_set) {
                    NET_LOG_ERROR(
                        m_logger,
                        "Failed to set non-blocking socket to listen fd, server will not start: {}",
                        err.value()
                    );
                }
                std::cerr << std::format(
                    "Failed to set non-blocking socket to listen fd, server will not start: {}\n",
                    err.value()
                );
                return;
            }
            EventHandler::SharedPtr server_event_handler = std::make_shared<EventHandler>();
            server_event_handler->m_on_read = [this](int server_fd) {
                while (true) {
                    addressResolver::address client_addr;
                    int client_fd = ::accept(server_fd, &client_addr.m_addr, &client_addr.m_addr_len);
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to accept RemoteTarget: {}", get_error_msg());
                        }
                        std::cerr << std::format("Failed to accept RemoteTarget: {}\n", get_error_msg());
                        continue;
                    }
                    auto err = set_non_blocking_socket(client_fd);
                    if (err.has_value()) {
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to set non-blocking socket: {}", err.value());
                        }
                        std::cerr << std::format("Failed to set non-blocking socket: {}\n", err.value());
                        return;
                    }
                    add_remote_event(client_fd);
                }
            };
            server_event_handler->m_on_error = [this](int server_fd) {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Error on server socket: {}", get_error_msg());
                }
                std::cerr << std::format("Error on server socket: {}\n", get_error_msg());
                this->close();
            };
            auto server_event = std::make_shared<Event>(m_listen_fd, server_event_handler);
            m_event_loop->add_event(server_event);
            while (!m_stop) {
                try {
                    m_event_loop->wait_for_events();
                } catch (std::runtime_error& e) {
                    std::cerr << std::format("Failed to waiting for events: {}\n", get_error_msg()) << std::endl;
                }
            }
        });
    } else {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                addressResolver::address client_addr;
                int client_fd = ::accept(m_listen_fd, &client_addr.m_addr, &client_addr.m_addr_len);
                if (client_fd == -1) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to accept RemoteTarget: {}", get_error_msg());
                    }
                    std::cerr << std::format("Failed to accept RemoteTarget: {}\n", get_error_msg());
                    continue;
                }
                auto remote = create_remote(client_fd);
                {
                    std::unique_lock<std::shared_mutex> lock(m_remotes_mutex);
                    m_remotes.insert({ client_fd, remote });
                }
                if (m_thread_pool) {
                    m_thread_pool->submit([this, client_fd]() {
                        RemoteTarget remote;
                        {
                            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
                            remote = m_remotes.at(client_fd);
                        }
                        handle_connection(remote);
                    });
                } else {
                    auto unused = std::async(std::launch::async, [this, client_fd]() {
                        RemoteTarget remote;
                        {
                            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
                            remote = m_remotes.at(client_fd);
                        }
                        handle_connection(remote);
                    });
                }
            }
        });
    }
    return std::nullopt;
}

std::optional<std::string> TcpServer::read(std::vector<uint8_t>& data, const RemoteTarget& remote) {
    ssize_t num_bytes;
    data.clear();
    do {
        std::vector<uint8_t> read_buffer(1024);
        num_bytes = ::recv(remote.m_client_fd, read_buffer.data(), read_buffer.size(), MSG_NOSIGNAL);
        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (data.size() <= 0) {
                    if (m_logger_set) {
                        NET_LOG_ERROR(
                            m_logger,
                            "Failed to read from socket {} (Epoll) : {}",
                            remote.m_client_fd,
                            get_error_msg()
                        );
                    }
                    const_cast<RemoteTarget&>(remote).m_status = false;
                    return get_error_msg();
                }
                break;
            } else {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Failed to read from socket {} : {}", remote.m_client_fd, get_error_msg());
                }
                const_cast<RemoteTarget&>(remote).m_status = false;
                return get_error_msg();
            }
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
            }
            const_cast<RemoteTarget&>(remote).m_status = false;
            return "Connection reset by peer while reading";
        }
        if (num_bytes > 0) {
            read_buffer.resize(num_bytes);
            std::copy(read_buffer.begin(), read_buffer.end(), std::back_inserter(data));
        }
    } while (num_bytes > 0 && m_event_loop);
    return std::nullopt;
}

std::optional<std::string> TcpServer::write(const std::vector<uint8_t>& data, const RemoteTarget& remote) {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(data.size() > 0 && "Data buffer is empty");
    ssize_t num_bytes;
    std::size_t bytes_has_send = 0;
    do {
        num_bytes =
            ::send(remote.m_client_fd, data.data() + bytes_has_send, data.size() - bytes_has_send, MSG_NOSIGNAL);
        if (num_bytes == -1) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to write to socket {} : {}", remote.m_client_fd, get_error_msg());
            }
            const_cast<RemoteTarget&>(remote).m_status = false;
            return get_error_msg();
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_WARN(m_logger, "Connection reset by peer while writting");
            }
            const_cast<RemoteTarget&>(remote).m_status = false;
            return "Connection reset by peer while writting";
        }
        bytes_has_send += num_bytes;
    } while (num_bytes > 0 && m_event_loop && bytes_has_send < data.size());
    return std::nullopt;
}

void TcpServer::handle_connection(const RemoteTarget& remote) {
    while (m_status == SocketStatus::LISTENING && remote.m_status) {
        m_accept_handler(remote);
    }
    {
        std::unique_lock<std::shared_mutex> lock(m_remotes_mutex);
        m_remotes.erase(remote.m_client_fd);
    }
}

RemoteTarget TcpServer::create_remote(int remote_fd) {
    RemoteTarget remote;
    remote.m_client_fd = remote_fd;
    remote.m_status = true;
    return remote;
}

void TcpServer::try_erase_remote(int remote_fd) {
    // erase expired remotes
    auto& remote = m_remotes.at(remote_fd);
    if (remote.m_ref_count != 0 || remote.m_status == true) {
        return;
    }
    m_event_loop->remove_event(remote_fd);
    {
        std::unique_lock<std::shared_mutex> lock(m_remotes_mutex);
        if (m_remotes.at(remote_fd).m_ref_count <= 0) {
            m_remotes.erase(remote_fd);
        }
    }
}

std::shared_ptr<TcpServer> TcpServer::get_shared() {
    return std::static_pointer_cast<TcpServer>(SocketServer::get_shared());
}

void TcpServer::add_remote_event(int client_fd) {
    auto client_event_handler = std::make_shared<EventHandler>();
    client_event_handler->m_on_read = [this](int client_fd) {
        if (!this->m_on_read) {
            return;
        }
        {
            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
            RemoteTarget& remote_target = m_remotes.at(client_fd);
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_read(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_read(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            }
        }
    };
    client_event_handler->m_on_write = [this](int client_fd) {
        if (!this->m_on_write) {
            return;
        }
        {
            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
            RemoteTarget& remote_target = m_remotes.at(client_fd);
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_write(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_write(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            }
        }
    };
    client_event_handler->m_on_error = [this](int client_fd) {
        if (!this->m_on_error) {
            return;
        };
        {
            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
            RemoteTarget& remote_target = m_remotes.at(client_fd);
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_error(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
                    remote_target.m_ref_count += 1;
                    this->m_on_error(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            }
        }
    };
    auto client_event = std::make_shared<Event>(client_fd, client_event_handler);
    auto remote = create_remote(client_fd);
    {
        std::unique_lock<std::shared_mutex> lock(m_remotes_mutex);
        m_remotes.insert({ client_fd, remote });
    }
    if (m_on_accept) {
        try {
            m_on_accept(m_remotes.at(client_fd));
        } catch (const std::exception& e) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to execute on accept: {}", e.what());
            }
            std::cerr << std::format("Failed to execute on accept: {}\n", e.what());
            return;
        }
    }
    m_event_loop->add_event(client_event);
}

} // namespace net