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

class SSLClient: public TcpClient {
public:
    SSLClient(
        std::shared_ptr<SSLContext> ctx,
        const std::string& ip,
        const std::string& service,
    );

    ~SSLClient();

    SSLClient(const SSLClient&) = delete;
    SSLClient(SSLClient&&) = default;
    SSLClient& operator=(const SSLClient&) = delete;
    SSLClient& operator=(SSLClient&&) = default;

    std::optional<std::string> write(const std::vector<uint8_t>& data) override;

    std::optional<std::string> read(std::vector<uint8_t>& data) override;

    std::optional<std::string> connect() override;

    std::optional<std::string> close() override;

protected:
    std::shared_ptr<TcpClient> m_client;
    std::shared_ptr<SSL> m_ssl;
    std::shared_ptr<SSLContext> m_ctx;
};

} // namespace net
