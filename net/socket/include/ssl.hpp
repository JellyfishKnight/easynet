#pragma once

#include "tcp.hpp"
#include <cstdint>
#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <optional>
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

    virtual ~SSLSession();

    SSLSession(const SSLSession&) = delete;
    SSLSession(SSLSession&&) = default;
    SSLSession& operator=(const SSLSession&) = delete;
    SSLSession& operator=(SSLSession&&) = default;

    virtual std::optional<std::string> handshake();

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data);

    virtual std::optional<std::string> read(std::vector<uint8_t>& data);

    virtual std::optional<std::string> close();

    virtual std::optional<std::string> set_socket(int fd);

protected:
    std::string get_ssl_error();

    std::shared_ptr<SSL> m_ssl;
    std::shared_ptr<SSLContext> m_ctx;

    bool m_closed;
};

class SSLClient: public SSLSession {
public:
    SSLClient(std::shared_ptr<SSLContext> ctx);

    ~SSLClient();

    SSLClient(const SSLClient&) = delete;
    SSLClient(SSLClient&&) = default;
    SSLClient& operator=(const SSLClient&) = delete;
    SSLClient& operator=(SSLClient&&) = default;

    std::optional<std::string> set_socket(std::shared_ptr<TcpClient> client);

    std::optional<std::string> handshake();

    std::optional<std::string> write(const std::vector<uint8_t>& data);

    std::optional<std::string> read(std::vector<uint8_t>& data);

    std::optional<std::string> close();

protected:
    std::shared_ptr<TcpClient> m_client;
};

} // namespace net
