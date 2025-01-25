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

struct SSLConnection: public Connection {
    SSLConnection(): Connection() {}

    std::shared_ptr<SSL> m_ssl;
};

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

    static std::shared_ptr<SSLContext> create() {
        return std::make_shared<SSLContext>();
    }

private:
    std::shared_ptr<SSL_CTX> m_ctx;
};

class SSLClient: public TcpClient {
public:
    SSLClient(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service);

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

class SSLServer: public TcpServer {
public:
    SSLServer(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service);

    ~SSLServer();

    SSLServer(const SSLServer&) = delete;
    SSLServer(SSLServer&&) = default;
    SSLServer& operator=(const SSLServer&) = delete;
    SSLServer& operator=(SSLServer&&) = default;

    std::optional<std::string> close() override;

    std::optional<std::string> listen() override;

    std::optional<std::string> start() override;

protected:
    std::optional<std::string>
    read(std::vector<uint8_t>& data, Connection::SharedPtr conn) override;

    std::optional<std::string>
    write(const std::vector<uint8_t>& data, Connection::SharedPtr conn) override;

    void handle_connection(Connection::SharedPtr conn) override;

    std::shared_ptr<SSLContext> m_ctx;
};

} // namespace net
