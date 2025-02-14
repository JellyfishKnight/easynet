#pragma once

#include "address_resolver.hpp"
#include "defines.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <unistd.h>

namespace net {

class RemoteTarget {
public:
    NET_DECLARE_PTRS(RemoteTarget)

    RemoteTarget(int fd): m_client_fd(fd) {}

    virtual ~RemoteTarget() {
        if (m_status.load()) {
            ::close(m_client_fd);
        }
    }

    bool is_active() const {
        return m_status.load();
    }

    void close_remote() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status.load()) {
            m_status.store(false);
            ::close(m_client_fd);
        }
    }

    int fd() const {
        return m_client_fd;
    }

private:
    int m_client_fd;
    std::atomic<bool> m_status = false;
    std::mutex m_mutex;
};

class RemotePool {
public:
    RemotePool() = default;

    void add_remote(int fd) {
        std::lock_guard<std::mutex> lock(m_mtx);
        remotes[fd] = std::make_shared<RemoteTarget>(fd);
    }

    void remove_remote(int fd) {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = remotes.find(fd);
        if (it != remotes.end()) {
            it->second->close_remote();
            remotes.erase(it);
        }
    }

    RemoteTarget::SharedPtr get_remote(int fd) {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = remotes.find(fd);
        if (it != remotes.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    std::map<int, std::shared_ptr<RemoteTarget>> remotes;
    std::mutex m_mtx;
};

} // namespace net
