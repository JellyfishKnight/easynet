#pragma once

#include "address_resolver.hpp"
#include "defines.hpp"
#include <cstdint>
#include <memory>
#include <sys/epoll.h>

namespace net {

struct RemoteTarget: std::enable_shared_from_this<RemoteTarget> {
    NET_DECLARE_PTRS(RemoteTarget)

    RemoteTarget() = default;

    int m_client_fd;

    bool m_status = false;

    virtual ~RemoteTarget() = default;
};

} // namespace net
