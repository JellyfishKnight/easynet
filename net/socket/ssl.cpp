#include "ssl.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
#include "tcp.hpp"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <cstdint>
#include <memory>
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

std::shared_ptr<SSLClient> SSLClient::get_shared() {
    return std::static_pointer_cast<SSLClient>(TcpClient::get_shared());
}

SSLServer::SSLServer(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service):
    TcpServer(ip, service),
    m_ctx(std::move(ctx)) {
    this->on_accept([this](const RemoteTarget& remote) {
        this->m_thread_pool->submit([this, &remote]() {
            auto& ssl = m_ssls.at(remote.m_client_fd);
            int err = SSL_accept(ssl.get());
            if (err <= 0) {
                err = SSL_get_error(ssl.get(), err);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    // int count = 0;
                    while ((SSL_accept(ssl.get()) <= 0))
                        ;
                } else {
                    if (m_logger_set) {
                        NET_LOG_ERROR(m_logger, "Failed to establish SSL connection");
                    }
                    std::cerr << std::format("Failed to establish SSL connection\n");
                    const_cast<RemoteTarget&>(remote).m_status = false;
                    return;
                }
            }
        });
    });
}

std::optional<std::string> SSLServer::listen() {
    auto opt = TcpServer::listen();
    if (opt.has_value()) {
        return opt.value();
    }
    return std::nullopt;
}

std::optional<std::string> SSLServer::read(std::vector<uint8_t>& data, const RemoteTarget& remote) {
    auto& ssl = m_ssls.at(remote.m_client_fd);
    while (SSL_get_state(ssl.get()) != TLS_ST_OK)
        ;
    int num_bytes;
    data.clear();
    do {
        std::vector<uint8_t> read_buffer(1024);
        num_bytes = SSL_read(ssl.get(), read_buffer.data(), read_buffer.size());
        if (num_bytes == -1) {
            int ssl_error = SSL_get_error(ssl.get(), num_bytes);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
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
                NET_LOG_ERROR(m_logger, "Connection reset by peer while reading");
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

std::optional<std::string> SSLServer::write(const std::vector<uint8_t>& data, const RemoteTarget& remote) {
    assert(m_status == SocketStatus::LISTENING && "Server is not listening");
    assert(data.size() > 0 && "Data buffer is empty");
    auto& ssl = m_ssls.at(remote.m_client_fd);
    while (SSL_get_state(ssl.get()) != TLS_ST_OK)
        ;
    int num_bytes;
    std::size_t bytes_has_send = 0;
    do {
        num_bytes = SSL_write(ssl.get(), data.data(), data.size());
        if (num_bytes == -1) {
            if (m_logger_set) {
                NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
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
    SSL_set_fd(m_ssls[remote_fd].get(), remote_fd);
    SSL_set_accept_state(m_ssls[remote_fd].get());
    return TcpServer::create_remote(remote_fd);
}

std::shared_ptr<SSLServer> SSLServer::get_shared() {
    return std::dynamic_pointer_cast<SSLServer>(TcpServer::get_shared());
}

} // namespace net