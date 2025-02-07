#pragma once

#include "address_resolver.hpp"
#include "defines.hpp"
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

struct RemoteTarget: std::enable_shared_from_this<RemoteTarget> {
    NET_DECLARE_PTRS(RemoteTarget)

    RemoteTarget() = default;

    int m_client_fd;

    bool m_status = false;

    virtual ~RemoteTarget() = default;
};

} // namespace net
