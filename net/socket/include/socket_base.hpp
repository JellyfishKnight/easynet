#pragma once

#include "address_resolver.hpp"
#include "event_loop.hpp"
#include "logger.hpp"
#include "remote_target.hpp"
#include "thread_pool.hpp"
#include "timer.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net {

enum class SocketType : uint8_t {
    TCP = 1,
    UDP,
    RAW,
};

enum class SocketStatus { CONNECTED = 0, LISTENING, DISCONNECTED };

class SocketClient: std::enable_shared_from_this<SocketClient> {
public:
    NET_DECLARE_PTRS(SocketClient)

    SocketClient() = default;

    SocketClient(const SocketClient&) = delete;

    SocketClient& operator=(const SocketClient&) = delete;

    SocketClient(SocketClient&&) = delete;

    SocketClient& operator=(SocketClient&&) = delete;

    virtual ~SocketClient() = default;

    virtual std::optional<NetError> connect(std::size_t time_out = 0) = 0;

    virtual std::optional<NetError> connect_with_retry(std::size_t time_out = 0, std::size_t retry_time_limit = 0) = 0;

    std::optional<NetError> start_event_loop();

    virtual std::optional<NetError> close() = 0;

    std::shared_ptr<SocketClient> get_shared();

    int get_fd() const;

    SocketType type() const;

    void set_logger(const utils::LoggerManager::Logger& logger);

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    virtual std::optional<NetError> read(std::vector<uint8_t>& data, std::size_t time_out = 0) = 0;

    virtual std::optional<NetError> write(const std::vector<uint8_t>& data, std::size_t time_out = 0) = 0;

protected:
    std::optional<NetError> set_non_blocking_socket(int fd);

    int m_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    SocketStatus m_status;
    SocketType m_socket_type;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

class SocketServer: std::enable_shared_from_this<SocketServer> {
    using CallBack = std::function<void(RemoteTarget::SharedPtr remote)>;

public:
    NET_DECLARE_PTRS(SocketServer)

    SocketServer() = default;

    SocketServer(const SocketServer&) = delete;

    SocketServer& operator=(const SocketServer&) = delete;

    SocketServer(SocketServer&&) = delete;

    SocketServer& operator=(SocketServer&&) = delete;

    virtual ~SocketServer() = default;

    virtual std::optional<NetError> listen() = 0;

    virtual std::optional<NetError> close() = 0;

    virtual std::optional<NetError> start() = 0;

    std::shared_ptr<SocketServer> get_shared();

    int get_fd() const;

    void enable_thread_pool(std::size_t worker_num);

    std::optional<NetError> enable_event_loop(EventLoopType type = EventLoopType::EPOLL, int time_out = -1);

    void set_logger(const utils::LoggerManager::Logger& logger);

    SocketType type() const;

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    void on_read(CallBack handler);

    void on_write(CallBack handler);

    void on_error(CallBack handler);

    void on_accept(CallBack handler);

    void on_start(CallBack handler);

    virtual std::optional<NetError> read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) = 0;

    virtual std::optional<NetError> write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) = 0;

protected:
    virtual void handle_connection(RemoteTarget::SharedPtr remote) = 0;

    virtual RemoteTarget::SharedPtr create_remote(int remote_fd) = 0;

    virtual void add_remote_event(int fd) = 0;


    std::optional<NetError> set_non_blocking_socket(int fd);

    int m_listen_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    CallBack m_accept_handler;
    CallBack m_on_read;
    CallBack m_on_write;
    CallBack m_on_error;
    CallBack m_on_accept;

    RemotePool m_remotes;

    utils::ThreadPool::SharedPtr m_thread_pool;
    EventLoop::SharedPtr m_event_loop;

    bool m_stop;

    std::thread m_accept_thread;

    SocketStatus m_status;
    SocketType m_socket_type;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

} // namespace net