#pragma once

#include "defines.hpp"
#include "event_loop.hpp"
#include "remote_target.hpp"
#include <memory>
#include <openssl/core.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <string>

namespace net {

class SSLContext {
public:
    NET_DECLARE_PTRS(SSLContext)

    SSLContext() {
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

    ~SSLContext() {
        ERR_free_strings();
        EVP_cleanup();
    }

    SSLContext(const SSLContext&) = delete;
    SSLContext(SSLContext&&) = default;
    SSLContext& operator=(const SSLContext&) = delete;
    SSLContext& operator=(SSLContext&&) = default;

    void set_certificates(const std::string& cert_file, const std::string& key_file) {
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

    std::shared_ptr<SSL_CTX> get() {
        return m_ctx;
    }

    static std::shared_ptr<SSLContext> create() {
        return std::make_shared<SSLContext>();
    }

private:
    inline static bool inited = false;

    std::shared_ptr<SSL_CTX> m_ctx;
};

class SSLRemoteTarget: public RemoteTarget {
public:
    NET_DECLARE_PTRS(SSLRemoteTarget)

    explicit SSLRemoteTarget(int fd, std::shared_ptr<SSL> ssl): RemoteTarget(fd), m_ssl(ssl) {}

    std::shared_ptr<SSL> get_ssl() {
        return m_ssl;
    }

    bool is_ssl_handshaked() {
        return m_ssl_handshaked;
    }

    void set_ssl_handshaked(bool handshaked) {
        m_ssl_handshaked = handshaked;
    }

    void close_remote() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status.load()) {
            m_status.store(false);
            SSL_shutdown(m_ssl.get());
            ::close(m_client_fd);
        }
    }

protected:
    std::shared_ptr<SSL> m_ssl;
    bool m_ssl_handshaked = false;
};

class SSLEvent: public Event {
public:
    NET_DECLARE_PTRS(SSLRemoteTarget)

    explicit SSLEvent(int fd, std::shared_ptr<EventHandler> handler, std::shared_ptr<SSL> ssl):
        Event(fd, handler),
        m_ssl(ssl) {}

    std::shared_ptr<SSL> get_ssl() {
        return m_ssl;
    }

    bool is_ssl_handshaked() {
        return m_ssl_handshaked;
    }

    void set_ssl_handshaked(bool handshaked) {
        m_ssl_handshaked = handshaked;
    }

    void close_remote() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status.load()) {
            m_status.store(false);
            SSL_shutdown(m_ssl.get());
            ::close(m_client_fd);
        }
    }

protected:
    std::shared_ptr<SSL> m_ssl;
    bool m_ssl_handshaked = false;
};

} // namespace net