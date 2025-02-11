#pragma once

#include "defines.hpp"
#include "remote_target.hpp"
#include "tcp.hpp"
#include <cstdint>
#include <memory>
#include <openssl/core.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/types.h>
#include <optional>
#include <string>
#include <vector>

namespace net {

class SSLContext: std::enable_shared_from_this<SSLContext> {
public:
    NET_DECLARE_PTRS(SSLContext)

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
    inline static bool inited = false;

    std::shared_ptr<SSL_CTX> m_ctx;
};

class SSLClient: public TcpClient {
public:
    NET_DECLARE_PTRS(SSLClient)

    SSLClient(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service);

    ~SSLClient() = default;

    SSLClient(const SSLClient&) = delete;
    SSLClient(SSLClient&&) = delete;
    SSLClient& operator=(const SSLClient&) = delete;
    SSLClient& operator=(SSLClient&&) = delete;

    std::optional<std::string> write(const std::vector<uint8_t>& data) override;

    std::optional<std::string> read(std::vector<uint8_t>& data) override;

    std::optional<std::string> connect() override;

    std::optional<std::string> close() override;

    std::shared_ptr<SSLClient> get_shared();

protected:
    std::shared_ptr<SSL> m_ssl;
    std::shared_ptr<SSLContext> m_ctx;
};

class SSLServer: public TcpServer {
public:
    NET_DECLARE_PTRS(SSLServer)

    SSLServer(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service);

    ~SSLServer() = default;

    SSLServer(const SSLServer&) = delete;
    SSLServer(SSLServer&&) = delete;
    SSLServer& operator=(const SSLServer&) = delete;
    SSLServer& operator=(SSLServer&&) = delete;

    std::optional<std::string> close() override;

    std::optional<std::string> listen() override;

    std::shared_ptr<SSLServer> get_shared();

    std::optional<std::string> read(std::vector<uint8_t>& data, const RemoteTarget& remote) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data, const RemoteTarget& remote) override;

protected:
    void handle_connection(const RemoteTarget& remote) override;

    RemoteTarget create_remote(int remote_fd) override;

    std::shared_ptr<SSLContext> m_ctx;
    std::map<int, std::shared_ptr<SSL>> m_ssls;
};

} // namespace net
