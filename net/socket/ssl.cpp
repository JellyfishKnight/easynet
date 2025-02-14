#include "ssl.hpp"
#include "defines.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
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
#include <utility>
#include <vector>

namespace net {

enum class SSLMethod : uint8_t {

};

SSLContext::SSLContext() {
    if (!inited) [[unlikely]] {
        inited = true;
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
    }
    m_ctx = std::shared_ptr<SSL_CTX>(SSL_CTX_new(TLS_method()), [](SSL_CTX* ctx) { SSL_CTX_free(ctx); });
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
    auto opt = TcpServer::listen();
    if (opt.has_value()) {
        return opt.value();
    }
    return std::nullopt;
}

std::optional<std::string> SSLServer::read(std::vector<uint8_t>& data, const RemoteTarget& remote) {
    auto& ssl = m_ssls.at(remote.m_client_fd);
    int num_bytes;
    data.clear();
    int i = 0;
    do {
        std::vector<uint8_t> read_buffer(1024);
        num_bytes = SSL_read(ssl.get(), read_buffer.data(), read_buffer.size());
        std::cout << std::format("Reading fd {}, times {} \n", remote.m_client_fd, i++) << std::endl;
        if (num_bytes == -1) {
            int ssl_error = SSL_get_error(ssl.get(), num_bytes);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                if (data.size() <= 0) {
                    const_cast<RemoteTarget&>(remote).m_status = false;
                    return get_error_msg();
                }
                break;
            } else if (ssl_error == SSL_ERROR_SYSCALL) {
                if (m_logger_set) {
                    NET_LOG_ERROR(
                        m_logger,
                        "Connection reset by peer while reading",
                        remote.m_client_fd,
                        ERR_error_string(ssl_error, nullptr)
                    );
                }
                const_cast<RemoteTarget&>(remote).m_status = false;
                return "Connection reset by peer while reading";
            } else {
                if (m_logger_set) {
                    NET_LOG_ERROR(
                        m_logger,
                        "Failed to read from socket {} : {}",
                        remote.m_client_fd,
                        ERR_error_string(ssl_error, nullptr)
                    );
                }
                const_cast<RemoteTarget&>(remote).m_status = false;
                return ERR_error_string(ssl_error, nullptr);
            }
        }
        if (num_bytes == 0) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Connection reset by peer while reading");
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

std::optional<std::string> SSLServer::write(const std::vector<uint8_t>& data, const RemoteTarget& remote) {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(data.size() > 0 && "Data buffer is empty");
    auto& ssl = m_ssls.at(remote.m_client_fd);
    int num_bytes;
    std::size_t bytes_has_send = 0;
    do {
        num_bytes = SSL_write(ssl.get(), data.data(), data.size());
        if (num_bytes == -1) {
            auto err = SSL_get_error(ssl.get(), num_bytes);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                break;
            } else if (err == SSL_ERROR_SYSCALL) {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Connection reset by peer while writting");
                }
                const_cast<RemoteTarget&>(remote).m_status = false;
                return "Connection reset by peer while writting";
            } else {
                if (m_logger_set) {
                    NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", ERR_error_string(err, nullptr));
                }
                const_cast<RemoteTarget&>(remote).m_status = false;
                return ERR_error_string(err, nullptr);
            }
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

std::optional<std::string> SSLServer::close() {
    std::for_each(m_remotes.begin(), m_remotes.end(), [this](auto& conn) {
        SSL_shutdown(m_ssls.at(conn.second.m_client_fd).get());
    });
    return TcpServer::close();
}

void SSLServer::handle_connection(const RemoteTarget& conn) {
    auto& ssl = m_ssls.at(conn.m_client_fd);
    if (SSL_accept(ssl.get()) <= 0) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to establish SSL connection");
        }
        std::cerr << std::format("Failed to establish SSL connection\n");
        return;
    }
    TcpServer::handle_connection(conn);
}

RemoteTarget SSLServer::create_remote(int remote_fd) {
    m_ssls[remote_fd] = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    m_ssl_handshakes[remote_fd] = false;
    SSL_set_fd(m_ssls[remote_fd].get(), remote_fd);
    return TcpServer::create_remote(remote_fd);
}

std::shared_ptr<SSLServer> SSLServer::get_shared() {
    return std::dynamic_pointer_cast<SSLServer>(TcpServer::get_shared());
}

bool SSLServer::handle_ssl_handshake(const RemoteTarget& remote) {
    auto& ssl = m_ssls.at(remote.m_client_fd);
    if (m_ssl_handshakes.at(remote.m_client_fd)) {
        return true;
    }
    int err_code = SSL_accept(ssl.get());
    if (err_code <= 0) {
        err_code = SSL_get_error(ssl.get(), err_code);
        if (err_code == SSL_ERROR_WANT_READ || err_code == SSL_ERROR_WANT_WRITE) {
            return false;
        }
        const_cast<RemoteTarget&>(remote).m_status = false;
        return false;
    } else {
        m_ssl_handshakes[remote.m_client_fd] = true;
        return false;
    }
}

void SSLServer::try_erase_remote(int remote_fd) {
    // erase expired remotes
    auto& remote = m_remotes.at(remote_fd);
    if (remote.m_ref_count != 0 || remote.m_status == true) {
        std::cout << std::format(
            "Erase {} Failed Ref count: {}, status: {}\n",
            remote_fd,
            remote.m_ref_count,
            remote.m_status
        );
        return;
    }
    m_event_loop->remove_event(remote_fd);
    {
        std::unique_lock<std::shared_mutex> lock(m_remotes_mutex);
        m_ssls.erase(remote_fd);
        m_ssl_handshakes.erase(remote_fd);
        if (m_remotes.at(remote_fd).m_ref_count <= 0) {
            m_remotes.erase(remote_fd);
        }
    }
    std::cout << std::format("Erase {} Success\n", remote_fd);
}

void SSLServer::add_remote_event(int client_fd) {
    auto client_event_handler = std::make_shared<EventHandler>();
    client_event_handler->m_on_read = [this](int client_fd) {
        if (!this->m_on_read) {
            return;
        }
        {
            std::shared_lock<std::shared_mutex> lock(m_remotes_mutex);
            RemoteTarget& remote_target = m_remotes.at(client_fd);
            if (!handle_ssl_handshake(remote_target)) {
                return;
            }
            if (this->m_thread_pool) {
                remote_target.m_ref_count += 1;
                this->m_thread_pool->submit([this, &remote_target]() {
                    this->m_on_read(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                remote_target.m_ref_count += 1;
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
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
            if (!handle_ssl_handshake(remote_target)) {
                return;
            }
            if (this->m_thread_pool) {
                remote_target.m_ref_count += 1;
                this->m_thread_pool->submit([this, &remote_target]() {
                    this->m_on_write(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                remote_target.m_ref_count += 1;
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
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
                remote_target.m_ref_count += 1;
                this->m_thread_pool->submit([this, &remote_target]() {
                    this->m_on_error(remote_target);
                    remote_target.m_ref_count -= 1;
                    try_erase_remote(remote_target.m_client_fd);
                });
            } else {
                remote_target.m_ref_count += 1;
                auto unused = std::async(std::launch::async, [this, &remote_target]() {
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