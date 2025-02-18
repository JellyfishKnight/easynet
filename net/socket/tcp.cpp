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
            auto error = GET_ERROR_MSG();
            NET_LOG_ERROR(m_logger, "Failed to create socket: {}", error.msg);
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

std::optional<NetError> TcpClient::connect(std::size_t time_out) {
    if (time_out == 0) {
        while (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
            if (errno == EALREADY || errno == EINPROGRESS) {
                continue;
            } else {
                return GET_ERROR_MSG();
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
            auto error = GET_ERROR_MSG();
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to connect to socket: {}", error.msg);
            }
            return error;
        }
        if (timer.timeout()) {
            auto error = GET_ERROR_MSG();
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Timeout to connect to socket, last error: {}", error.msg);
            }
            return NetError { NET_TIMEOUT_CODE,
                              std::format("Timeout to connect to socket, last error: {}", error.msg) };
        }
    }
    m_status = SocketStatus::CONNECTED;
    return std::nullopt;
}

std::optional<NetError> TcpClient::connect_with_retry(std::size_t time_out, std::size_t retry_time_limit) {
    std::size_t tried_time = 0;
    while (true) {
        if (tried_time++ >= retry_time_limit) {
            auto error = GET_ERROR_MSG();
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to connect to socket: {}", error.msg);
            }
            return error;
        }
        auto ret = this->connect(time_out);
        if (!ret.has_value()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<NetError> TcpClient::close() {
    if (::close(m_fd) == -1) {
        auto error = GET_ERROR_MSG();
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", error.msg);
        }
        return error;
    }
    m_status = SocketStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<NetError> TcpClient::read(std::vector<uint8_t>& data, std::size_t time_out) {
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
            return NetError { NET_TIMEOUT_CODE, "Timeout to read from socket" };
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
            auto error = GET_ERROR_MSG();
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", error.msg);
            }
            return error;
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
            }
            return NetError { NET_CONNECTION_RESET_CODE, "Connection reset by peer while reading" };
        }
        buffer.resize(num_bytes);
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(data));
    };
    return std::nullopt;
}

std::optional<NetError> TcpClient::write(const std::vector<uint8_t>& data, std::size_t time_out) {
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
            return NetError { NET_TIMEOUT_CODE, "Timeout to write to socket" };
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
                    return NetError { NET_EARLY_END_OF_SOCKET, "early end of socket" };
                }
                auto error = GET_ERROR_MSG();
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", error.msg);
                }
                return error;
            }
            if (num_bytes == 0) {
                if (m_logger_set) {
                    NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
                }
                return NetError { NET_CONNECTION_RESET_CODE, "Connection reset by peer while writing" };
            }
            bytes_has_send += num_bytes;
            if (bytes_has_send >= data.size()) {
                return std::nullopt;
            }
        };
        return std::nullopt;
    }
}

std::shared_ptr<TcpClient> TcpClient::get_shared() {
    return std::static_pointer_cast<TcpClient>(SocketClient::get_shared());
}

TcpClient::~TcpClient() {
    if (m_status == SocketStatus::CONNECTED) {
        this->close();
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

std::optional<NetError> TcpServer::listen() {
    int opt = 1;
    auto ret = ::setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1) {
        auto error = GET_ERROR_MSG();
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to set socket options: {}", error.msg);
        }
        return error;
    }
    if (::bind(m_listen_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
        auto error = GET_ERROR_MSG();
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to bind socket: {}", error.msg);
        }
        return error;
    }
    if (::listen(m_listen_fd, 10) == -1) {
        auto error = GET_ERROR_MSG();
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to listen on socket: {}", error.msg);
        }
        return error;
    }

    m_status = SocketStatus::LISTENING;
    return std::nullopt;
}

std::optional<NetError> TcpServer::close() {
    m_stop = true;
    m_accept_thread.join();
    m_event_loop.reset();
    if (m_thread_pool) {
        m_thread_pool->stop();
        m_thread_pool.reset();
    }
    if (::close(m_listen_fd) == -1) {
        auto error = GET_ERROR_MSG();
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", error.msg);
        }
        return error;
    }
    m_status = SocketStatus::DISCONNECTED;
    return std::nullopt;
}

std::optional<NetError> TcpServer::start() {
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
                        err.value().msg
                    );
                }
                std::cerr << std::format(
                    "Failed to set non-blocking socket to listen fd, server will not start: {}\n",
                    err.value().msg
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
                        auto error = GET_ERROR_MSG();
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to accept RemoteTarget: {}", error.msg);
                        }
                        std::cerr << std::format("Failed to accept RemoteTarget: {}\n", error.msg);
                        continue;
                    }
                    auto err = set_non_blocking_socket(client_fd);
                    if (err.has_value()) {
                        if (m_logger_set) {
                            NET_LOG_ERROR(m_logger, "Failed to set non-blocking socket: {}", err.value().msg);
                        }
                        std::cerr << std::format("Failed to set non-blocking socket: {}\n", err.value().msg);
                        return;
                    }
                    add_remote_event(client_fd);
                }
            };
            server_event_handler->m_on_error = [this](int server_fd) {
                auto error = GET_ERROR_MSG();
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Error on server socket: {}", error.msg);
                }
                std::cerr << std::format("Error on server socket: {}\n", error.msg);
                this->close();
            };
            auto server_event = std::make_shared<Event>(m_listen_fd, server_event_handler);
            m_event_loop->add_event(server_event);
            while (!m_stop) {
                try {
                    m_event_loop->wait_for_events();
                } catch (std::runtime_error& e) {
                    std::cerr << std::format("Failed to waiting for events: {}\n", e.what()) << std::endl;
                }
            }
        });
    } else {
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                addressResolver::address client_addr;
                int client_fd = ::accept(m_listen_fd, &client_addr.m_addr, &client_addr.m_addr_len);
                if (client_fd == -1) {
                    auto error = GET_ERROR_MSG();
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to accept RemoteTarget: {}", error.msg);
                    }
                    std::cerr << std::format("Failed to accept RemoteTarget: {}\n", error.msg);
                    continue;
                }
                m_remotes.add_remote(create_remote(client_fd));
                if (m_thread_pool) {
                    m_thread_pool->submit([this, client_fd]() { handle_connection(m_remotes.get_remote(client_fd)); });
                } else {
                    auto unused = std::async(std::launch::async, [this, client_fd]() {
                        handle_connection(m_remotes.get_remote(client_fd));
                    });
                }
            }
        });
    }
    return std::nullopt;
}

std::optional<NetError> TcpServer::read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
    ssize_t num_bytes;
    data.clear();
    do {
        std::vector<uint8_t> read_buffer(1024);
        num_bytes = ::recv(remote->fd(), read_buffer.data(), read_buffer.size(), MSG_NOSIGNAL);
        if (num_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (data.size() <= 0) {
                    auto error = GET_ERROR_MSG();
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to read from socket {} (Epoll) : {}", remote->fd(), error.msg);
                    }
                    if (m_event_loop) {
                        m_event_loop->remove_event(remote->fd());
                    } else {
                        m_remotes.remove_remote(remote->fd());
                    }
                    return error;
                }
                break;
            } else {
                auto error = GET_ERROR_MSG();
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Failed to read from socket {} : {}", remote->fd(), error.msg);
                }
                if (m_event_loop) {
                    m_event_loop->remove_event(remote->fd());
                } else {
                    m_remotes.remove_remote(remote->fd());
                }
                return error;
            }
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
            }
            if (m_event_loop) {
                m_event_loop->remove_event(remote->fd());
            } else {
                m_remotes.remove_remote(remote->fd());
            }
            return NetError { NET_CONNECTION_RESET_CODE, "Connection reset by peer while reading" };
        }
        if (num_bytes > 0) {
            read_buffer.resize(num_bytes);
            std::copy(read_buffer.begin(), read_buffer.end(), std::back_inserter(data));
        }
    } while (num_bytes > 0 && m_event_loop);
    return std::nullopt;
}

std::optional<NetError> TcpServer::write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(data.size() > 0 && "Data buffer is empty");
    ssize_t num_bytes;
    std::size_t bytes_has_send = 0;
    do {
        num_bytes = ::send(remote->fd(), data.data() + bytes_has_send, data.size() - bytes_has_send, MSG_NOSIGNAL);
        if (num_bytes == -1) {
            auto error = GET_ERROR_MSG();
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to write to socket {} : {}", remote->fd(), error.msg);
            }
            if (m_event_loop) {
                m_event_loop->remove_event(remote->fd());
            } else {
                m_remotes.remove_remote(remote->fd());
            }
            return error;
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_WARN(m_logger, "Connection reset by peer while writting");
            }
            if (m_event_loop) {
                m_event_loop->remove_event(remote->fd());
            } else {
                m_remotes.remove_remote(remote->fd());
            }
            return NetError { NET_CONNECTION_RESET_CODE, "Connection reset by peer while writting" };
        }
        bytes_has_send += num_bytes;
    } while (num_bytes > 0 && m_event_loop && bytes_has_send < data.size());
    return std::nullopt;
}

void TcpServer::handle_connection(RemoteTarget::SharedPtr remote) {
    while (m_status == SocketStatus::LISTENING && remote->is_active()) {
        m_accept_handler(remote);
    }
}

RemoteTarget::SharedPtr TcpServer::create_remote(int remote_fd) {
    return std::make_shared<RemoteTarget>(remote_fd);
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
            auto event = m_event_loop->get_event(client_fd);
            if (event == nullptr) {
                return;
            }
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, event]() { this->m_on_read(std::move(event)); });
            } else {
                auto unused = std::async(std::launch::async, [this, event]() { this->m_on_read(std::move(event)); });
            }
        }
    };
    client_event_handler->m_on_write = [this](int client_fd) {
        if (!this->m_on_write) {
            return;
        }
        {
            auto event = m_event_loop->get_event(client_fd);
            if (event == nullptr) {
                return;
            }
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, event]() { this->m_on_write(std::move(event)); });
            } else {
                auto unused = std::async(std::launch::async, [this, event]() { this->m_on_write(std::move(event)); });
            }
        }
    };
    client_event_handler->m_on_error = [this](int client_fd) {
        if (!this->m_on_error) {
            return;
        };
        {
            auto event = m_event_loop->get_event(client_fd);
            if (event == nullptr) {
                return;
            }
            if (this->m_thread_pool) {
                this->m_thread_pool->submit([this, event]() { this->m_on_error(std::move(event)); });
            } else {
                auto unused = std::async(std::launch::async, [this, event]() { this->m_on_error(std::move(event)); });
            }
        }
    };
    auto client_event = std::make_shared<Event>(client_fd, client_event_handler);
    m_event_loop->add_event(client_event);
    if (m_on_accept) {
        try {
            m_on_accept(m_event_loop->get_event(client_fd));
        } catch (const std::exception& e) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to execute on accept: {}", e.what());
            }
            std::cerr << std::format("Failed to execute on accept: {}\n", e.what());
            return;
        }
    }
}

} // namespace net