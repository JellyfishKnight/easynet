#pragma once

#include "address_resolver.hpp"
#include <cstdint>
#include <memory>
#include <sys/epoll.h>

namespace net {

struct ConnectionKey {
    std::string m_ip;
    std::string m_service;

    bool operator==(const ConnectionKey& other) const {
        return m_ip == other.m_ip && m_service == other.m_service;
    }
};
} // namespace net

namespace std {
template<>
struct hash<net::ConnectionKey> {
    std::size_t operator()(const net::ConnectionKey& key) const {
        return std::hash<std::string>()(key.m_ip) ^ std::hash<std::string>()(key.m_service);
    }
};
} // namespace std

namespace net {

enum class ConnectionStatus { CONNECTED = 0, LISTENING, DISCONNECTED };

struct Connection: std::enable_shared_from_this<Connection> {
    using SharedPtr = std::shared_ptr<Connection>;

    Connection() = default;

    std::string m_client_ip;
    std::string m_client_service;
    std::string m_server_ip;
    std::string m_server_service;

    int m_client_fd;
    int m_server_fd;

    addressResolver::address m_addr;
    ConnectionStatus m_status = ConnectionStatus::DISCONNECTED;

    virtual ~Connection() = default;
};

} // namespace net
