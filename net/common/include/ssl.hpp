#pragma once

#include <cstdint>
#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <string>
#include <vector>

namespace net {

class SSLContext: std::enable_shared_from_this<SSLContext> {
public:
    SSLContext();

    ~SSLContext();

    SSLContext(const SSLContext&) = delete;
    SSLContext(SSLContext&&) = default;
    SSLContext& operator=(const SSLContext&) = delete;
    SSLContext& operator=(SSLContext&&) = default;

    void set_certificates(const std::string& cert_file, const std::string& key_file);

    std::shared_ptr<SSL_CTX> get();

private:
    std::shared_ptr<SSL_CTX> m_ctx;
};

class SSLSession {
public:
    SSLSession(std::shared_ptr<SSLContext> ctx);

    ~SSLSession();

    SSLSession(const SSLSession&) = delete;
    SSLSession(SSLSession&&) = default;
    SSLSession& operator=(const SSLSession&) = delete;
    SSLSession& operator=(SSLSession&&) = default;

    void set_socket(int fd);

    void handshake();

    void write(const std::vector<uint8_t>& data);

    void read(std::vector<uint8_t>& data);

    void close();

private:
    std::string get_ssl_error();

    std::shared_ptr<SSL> m_ssl;
};

} // namespace net
