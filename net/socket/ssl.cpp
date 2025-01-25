#include "ssl.hpp"
#include "socket.hpp"
#include "tcp.hpp"
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
    m_ctx(std::move(ctx)) {
    m_ssl = std::shared_ptr<SSL>(SSL_new(m_ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    if (m_ssl == nullptr) {
        throw std::runtime_error("Failed to create SSL object");
    }
    SSL_set_fd(m_ssl.get(), m_listen_fd);
}

std::optional<std::string> SSLServer::listen() {
    if (TcpServer::listen().has_value()) {
        return "Failed to listen on socket";
    }
    if (SSL_accept(m_ssl.get()) <= 0) {
        return "Failed to accept SSL connection";
    }
    return std::nullopt;
}

std::optional<std::string> SSLServer::close(const Connection& conn) {
    if (SSL_shutdown(m_ssl.get()) == 0) {
        return "Failed to shutdown SSL connection";
    }
    return TcpServer::close(conn);
}

std::optional<std::string> SSLServer::read(std::vector<uint8_t>& data, const Connection& conn) {
    int num_bytes = SSL_read(m_ssl.get(), data.data(), data.size());
    if (num_bytes <= 0) {
        return "Failed to read data";
    }
    data.resize(num_bytes);
    return std::nullopt;
}



} // namespace net