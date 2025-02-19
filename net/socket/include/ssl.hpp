#pragma once

#include "defines.hpp"
#include "remote_target.hpp"
#include "ssl_utils.hpp"
#include "tcp.hpp"
#include <cstddef>
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

class SSLClient: public TcpClient {
public:
    NET_DECLARE_PTRS(SSLClient)

    SSLClient(std::shared_ptr<SSLContext> ctx, const std::string& ip, const std::string& service);

    ~SSLClient() = default;

    SSLClient(const SSLClient&) = delete;
    SSLClient(SSLClient&&) = delete;
    SSLClient& operator=(const SSLClient&) = delete;
    SSLClient& operator=(SSLClient&&) = delete;

    std::optional<NetError> write(const std::vector<uint8_t>& data, std::size_t time_out = 0) override;

    std::optional<NetError> read(std::vector<uint8_t>& data, std::size_t time_out = 0) override;

    std::optional<NetError> connect(std::size_t time_out = 0) override;

    std::optional<NetError> connect_with_retry(std::size_t time_out, std::size_t retry_time_limit = 0) override;

    std::optional<NetError> close() override;

protected:
    std::optional<NetError> ssl_connect(std::size_t time_out = 0);

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

    std::optional<NetError> close() override;

    std::optional<NetError> listen() override;

    std::optional<NetError> read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) override;

    std::optional<NetError> write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) override;

protected:
    bool handle_ssl_handshake(RemoteTarget::SharedPtr remote);

    void handle_connection(RemoteTarget::SharedPtr remote) override;

    RemoteTarget::SharedPtr create_remote(int remote_fd) override;

    void add_remote_event(int fd) override;

    std::shared_ptr<SSLContext> m_ctx;
};

} // namespace net
