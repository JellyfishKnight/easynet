#pragma once

#include "defines.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <unistd.h>

namespace net {

class RemoteTarget {
public:
    NET_DECLARE_PTRS(RemoteTarget)

    explicit RemoteTarget(int fd): m_client_fd(fd) {}

    virtual ~RemoteTarget() {
        if (m_status.load()) {
            close_remote();
        }
    }

    bool is_active() const {
        return m_status.load();
    }

    virtual void close_remote() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status.load()) {
            m_status.store(false);
            ::close(m_client_fd);
        }
    }

    int fd() const {
        return m_client_fd;
    }

protected:
    int m_client_fd;
    std::atomic<bool> m_status = true;
    std::mutex m_mutex;
};

class RemotePool {
public:
    RemotePool() = default;

    void add_remote(std::shared_ptr<RemoteTarget> remote) {
        std::lock_guard<std::mutex> lock(m_mtx);
        remotes[remote->fd()] = remote;
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

    void iterate(std::function<void(RemoteTarget::SharedPtr)> func) {
        std::lock_guard<std::mutex> lock(m_mtx);
        for (auto& [_, remote]: remotes) {
            func(remote);
        }
    }

private:
    std::map<int, std::shared_ptr<RemoteTarget>> remotes;
    std::mutex m_mtx;
};

} // namespace net
