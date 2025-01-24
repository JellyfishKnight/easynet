#include "ssl.hpp"
#include "tcp.hpp"
#include <memory>
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

SSLSession::SSLSession(std::shared_ptr<SSLContext> ctx) {
    m_ssl = std::shared_ptr<SSL>(SSL_new(ctx->get().get()), [](SSL* ssl) { SSL_free(ssl); });
    if (m_ssl == nullptr) {
        throw std::runtime_error("Failed to create SSL session: " + get_ssl_error());
    }
    m_closed = true;
}

std::optional<std::string> SSLSession::handshake() {
    if (SSL_connect(m_ssl.get()) <= 0) {
        return "Failed to establish SSL connection: " + get_ssl_error();
    }
    m_closed = false;
    return std::nullopt;
}

std::optional<std::string> SSLSession::write(const std::vector<uint8_t>& data) {
    if (m_closed) {
        return "SSL session is closed";
    }
    if (SSL_write(m_ssl.get(), data.data(), data.size()) <= 0) {
        throw std::runtime_error("Failed to write data to SSL session: " + get_ssl_error());
    }
}

std::optional<std::string> SSLSession::read(std::vector<uint8_t>& data) {
    if (m_closed) {
        return "SSL session is closed";
    }
    data.resize(1024);
    int num_bytes = SSL_read(m_ssl.get(), data.data(), data.size());
    if (num_bytes <= 0) {
        return "Failed to read data from SSL session: " + get_ssl_error();
    }
    data.resize(num_bytes);
    return std::nullopt;
}

std::optional<std::string> SSLSession::close() {
    if (SSL_shutdown(m_ssl.get()) <= 0) {
        return "Failed to close SSL session: " + get_ssl_error();
    }
    m_closed = true;
    return std::nullopt;
}

std::string SSLSession::get_ssl_error() {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
    return buffer;
}

SSLSession::~SSLSession() {
    if (!m_closed) {
        close();
    }
}

std::optional<std::string> SSLSession::set_socket(int fd) {
    if (SSL_set_fd(m_ssl.get(), fd) <= 0) {
        return "Failed to set socket for SSL session: " + get_ssl_error();
    }
    return std::nullopt;
}
} // namespace net