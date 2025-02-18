#include "ssl.hpp"
#include "defines.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
#include "ssl_utils.hpp"
#include "tcp.hpp"
#include "timer.hpp"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace net {

SSLClient::SSLClient(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service):
    TcpClient(ip, service),
    m_ctx(std::move(ctx)) {
    m_ssl = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    if (m_ssl == nullptr) {
        throw std::runtime_error("Failed to create SSL object");
    }
    SSL_set_fd(m_ssl.get(), m_fd);
}

std::optional<std::string> SSLClient::ssl_connect(std::size_t time_out) {
    if (time_out == 0) {
        while (true) {
            int err = SSL_connect(m_ssl.get());
            if (err == 1) {
                return std::nullopt;
            }
            if (err == 0) {
                return "Failed to connect to server";
            }
            int ssl_error = SSL_get_error(m_ssl.get(), err);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                continue;
            }
            return ERR_error_string(ssl_error, nullptr);
        }
    }
    Timer timer;
    timer.set_timeout(std::chrono::milliseconds(time_out));
    timer.async_start_timing();
    while (true) {
        if (timer.timeout()) {
            return "Timeout to connect to server";
        }
        int err = SSL_connect(m_ssl.get());
        if (err == 1) {
            return std::nullopt;
        }
        if (err == 0) {
            return "Failed to connect to server";
        }
        int ssl_error = SSL_get_error(m_ssl.get(), err);
        if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
            continue;
        }
    }
}

std::optional<std::string> SSLClient::connect(std::size_t time_out) {
    auto opt = TcpClient::connect(time_out);
    if (opt.has_value()) {
        return opt.value();
    }
    return ssl_connect(time_out);
}

std::optional<std::string> SSLClient::connect_with_retry(std::size_t time_out, std::size_t retry_time_limit) {
    auto opt = TcpClient::connect_with_retry(time_out, retry_time_limit);
    if (opt.has_value()) {
        return opt.value();
    }
    std::size_t tried_time = 0;
    while (true) {
        if (tried_time++ >= retry_time_limit) {
            return "Failed to connect to server";
        }
        auto ret = ssl_connect(time_out);
        if (!ret.has_value()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<std::string> SSLClient::write(const std::vector<uint8_t>& data, std::size_t time_out) {
    assert(m_status == SocketStatus::CONNECTED && "Client is not connected");
    assert(data.size() > 0 && "Data buffer is empty");
    Timer timer;
    std::size_t bytes_has_send = 0;
    timer.reset();
    if (time_out != 0) {
        timer.set_timeout(std::chrono::milliseconds(time_out));
        timer.async_start_timing();
    }
    while (true) {
        if (timer.timeout()) {
            return "Timeout to write data";
        }
        auto err = SSL_write(m_ssl.get(), data.data(), data.size());
        if (err > 0) {
            bytes_has_send += err;
            if (bytes_has_send == data.size()) {
                return std::nullopt;
            }
        } else if (err == 0) {
            return "Connection reset by peer while writing";
        } else {
            int ssl_error = SSL_get_error(m_ssl.get(), err);
            if ((ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)) {
                if (bytes_has_send <= 0) {
                    continue;
                }
                break;
            }
            return ERR_error_string(ssl_error, nullptr);
        }
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<std::string> SSLClient::read(std::vector<uint8_t>& data, std::size_t time_out) {
    assert(m_status == SocketStatus::CONNECTED && "Client is not connected");
    data.clear();
    int num_bytes;
    Timer timer;
    timer.reset();
    if (time_out != 0) {
        timer.set_timeout(std::chrono::milliseconds(time_out));
        timer.async_start_timing();
    }
    while (true) {
        if (timer.timeout()) {
            return "Timeout to read data";
        }
        std::vector<uint8_t> buffer(1024);
        num_bytes = SSL_read(m_ssl.get(), buffer.data(), buffer.size());
        if (num_bytes <= 0) {
            auto ssl_err = SSL_get_error(m_ssl.get(), num_bytes);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
                if (data.size() <= 0) {
                    continue;
                }
                break;
            } else if (ssl_err == SSL_ERROR_SYSCALL) {
                return "Connection reset by peer while reading";
            } else {
                return ERR_error_string(ssl_err, nullptr);
            }
        }
        buffer.resize(num_bytes);
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(data));
    }

    return std::nullopt;
}

std::optional<std::string> SSLClient::close() {
    if (SSL_shutdown(m_ssl.get()) == 0) {
        return "Failed to shutdown SSL connection";
    }
    return TcpClient::close();
}

std::shared_ptr<SSLClient> SSLClient::get_shared() {
    return std::static_pointer_cast<SSLClient>(TcpClient::get_shared());
}

SSLServer::SSLServer(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service):
    TcpServer(ip, service),
    m_ctx(std::move(ctx)) {}

std::optional<std::string> SSLServer::listen() {
    return TcpServer::listen();
}

std::optional<std::string> SSLServer::read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
    auto ssl_remote = std::dynamic_pointer_cast<SSLRemoteTarget>(remote);
    int num_bytes;
    data.clear();
    do {
        std::vector<uint8_t> read_buffer(1024);
        num_bytes = SSL_read(ssl_remote->get_ssl().get(), read_buffer.data(), read_buffer.size());
        if (num_bytes == -1) {
            int ssl_error = SSL_get_error(ssl_remote->get_ssl().get(), num_bytes);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                if (data.size() <= 0) {
                    if (m_event_loop) {
                        m_event_loop->remove_event(remote->fd());
                    } else {
                        m_remotes.remove_remote(remote->fd());
                    }
                    return get_error_msg();
                }
                break;
            } else if (ssl_error == SSL_ERROR_SYSCALL) {
                if (m_logger_set) {
                    NET_LOG_ERROR(
                        m_logger,
                        "Connection reset by peer while reading",
                        ssl_remote->fd(),
                        ERR_error_string(ssl_error, nullptr)
                    );
                }
                if (m_event_loop) {
                    m_event_loop->remove_event(remote->fd());
                } else {
                    m_remotes.remove_remote(remote->fd());
                }
                return "Connection reset by peer while reading";
            } else {
                if (m_logger_set) {
                    NET_LOG_ERROR(
                        m_logger,
                        "Failed to read from socket {} : {}",
                        ssl_remote->fd(),
                        ERR_error_string(ssl_error, nullptr)
                    );
                }
                if (m_event_loop) {
                    m_event_loop->remove_event(remote->fd());
                } else {
                    m_remotes.remove_remote(remote->fd());
                }
                return ERR_error_string(ssl_error, nullptr);
            }
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Connection reset by peer while reading");
            }
            if (m_event_loop) {
                m_event_loop->remove_event(remote->fd());
            } else {
                m_remotes.remove_remote(remote->fd());
            }
            return "Connection reset by peer while reading";
        }
        if (num_bytes > 0) {
            read_buffer.resize(num_bytes);
            std::copy(read_buffer.begin(), read_buffer.end(), std::back_inserter(data));
        }
    } while (num_bytes > 0 && m_event_loop);
    return std::nullopt;
}

std::optional<std::string> SSLServer::write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(data.size() > 0 && "Data buffer is empty");
    auto ssl_remote = std::dynamic_pointer_cast<SSLRemoteTarget>(remote);
    int num_bytes;
    std::size_t bytes_has_send = 0;
    do {
        num_bytes = SSL_write(ssl_remote->get_ssl().get(), data.data(), data.size());
        if (num_bytes == -1) {
            auto err = SSL_get_error(ssl_remote->get_ssl().get(), num_bytes);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                break;
            } else if (err == SSL_ERROR_SYSCALL) {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Connection reset by peer while writting");
                }
                if (m_event_loop) {
                    m_event_loop->remove_event(remote->fd());
                } else {
                    m_remotes.remove_remote(remote->fd());
                }
                return "Connection reset by peer while writting";
            } else {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", ERR_error_string(err, nullptr));
                }
                if (m_event_loop) {
                    m_event_loop->remove_event(remote->fd());
                } else {
                    m_remotes.remove_remote(remote->fd());
                }
                return ERR_error_string(err, nullptr);
            }
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
            return "Connection reset by peer while writting";
        }
        bytes_has_send += num_bytes;
    } while (num_bytes > 0 && m_event_loop && bytes_has_send < data.size());
    return std::nullopt;
}

std::optional<std::string> SSLServer::close() {
    m_remotes.iterate([](auto remote) {
        auto ssl_remote = std::dynamic_pointer_cast<SSLRemoteTarget>(remote);
        SSL_shutdown(ssl_remote->get_ssl().get());
    });
    return TcpServer::close();
}

void SSLServer::handle_connection(RemoteTarget::SharedPtr remote) {
    auto ssl_remote = std::dynamic_pointer_cast<SSLRemoteTarget>(remote);

    if (SSL_accept(ssl_remote->get_ssl().get()) <= 0) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to establish SSL connection");
        }
        std::cerr << std::format("Failed to establish SSL connection\n");
        return;
    }
    TcpServer::handle_connection(ssl_remote);
}

RemoteTarget::SharedPtr SSLServer::create_remote(int remote_fd) {
    auto ssl = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    SSLRemoteTarget::SharedPtr remote = std::make_shared<SSLRemoteTarget>(remote_fd, ssl);
    SSL_set_fd(ssl.get(), remote_fd);
    return remote;
}

std::shared_ptr<SSLServer> SSLServer::get_shared() {
    return std::dynamic_pointer_cast<SSLServer>(TcpServer::get_shared());
}

bool SSLServer::handle_ssl_handshake(RemoteTarget::SharedPtr remote) {
    auto ssl_remote = std::dynamic_pointer_cast<SSLEvent>(remote);
    if (ssl_remote->is_ssl_handshaked()) {
        return true;
    }
    int err_code = SSL_accept(ssl_remote->get_ssl().get());
    if (err_code <= 0) {
        err_code = SSL_get_error(ssl_remote->get_ssl().get(), err_code);
        if (err_code == SSL_ERROR_WANT_READ || err_code == SSL_ERROR_WANT_WRITE) {
            return false;
        }
        ssl_remote->close_remote();
        return false;
    } else {
        ssl_remote->set_ssl_handshaked(true);
        return false;
    }
}

void SSLServer::add_remote_event(int client_fd) {
    auto client_event_handler = std::make_shared<EventHandler>();
    client_event_handler->m_on_read = [this](int client_fd) {
        if (!this->m_on_read) {
            return;
        }

        auto event = m_event_loop->get_event(client_fd);
        if (event == nullptr) {
            return;
        }
        if (!handle_ssl_handshake(event)) {
            return;
        }
        if (this->m_thread_pool) {
            this->m_thread_pool->submit([this, event]() { this->m_on_read(event); });
        } else {
            auto unused = std::async(std::launch::async, [this, event]() { this->m_on_read(event); });
        }
    };
    client_event_handler->m_on_write = [this](int client_fd) {
        if (!this->m_on_write) {
            return;
        }
        auto event = m_event_loop->get_event(client_fd);
        if (event == nullptr) {
            return;
        }
        if (!handle_ssl_handshake(event)) {
            return;
        }
        if (this->m_thread_pool) {
            this->m_thread_pool->submit([this, event]() { this->m_on_write(event); });
        } else {
            auto unused = std::async(std::launch::async, [this, event]() { this->m_on_write(event); });
        }
    };
    client_event_handler->m_on_error = [this](int client_fd) {
        if (!this->m_on_error) {
            return;
        };
        auto event = m_event_loop->get_event(client_fd);
        if (event == nullptr) {
            return;
        }
        if (this->m_thread_pool) {
            this->m_thread_pool->submit([this, event]() { this->m_on_error(event); });
        } else {
            auto unused = std::async(std::launch::async, [this, event]() { this->m_on_error(event); });
        }
    };
    auto ssl = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    auto client_event = std::make_shared<SSLEvent>(client_fd, client_event_handler, ssl);
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