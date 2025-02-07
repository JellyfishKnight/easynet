#pragma once

#include "address_resolver.hpp"
#include "event_loop.hpp"
#include "logger.hpp"
#include "remote_target.hpp"
#include "thread_pool.hpp"
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unordered_map>
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

    SocketClient(SocketClient&&) = default;

    SocketClient& operator=(SocketClient&&) = default;

    virtual ~SocketClient() = default;

    virtual std::optional<std::string> connect() = 0;

    virtual std::optional<std::string> close() = 0;

    std::shared_ptr<SocketClient> get_shared();

    int get_fd() const;

    SocketType type() const;

    void set_logger(const utils::LoggerManager::Logger& logger);

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    virtual std::optional<std::string> read(std::vector<uint8_t>& data) = 0;

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data) = 0;

protected:
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
    using CallBack = std::function<void(int client_fd)>;

public:
    NET_DECLARE_PTRS(SocketServer)

    SocketServer() = default;

    SocketServer(const SocketServer&) = delete;

    SocketServer& operator=(const SocketServer&) = delete;

    SocketServer(SocketServer&&) = default;

    SocketServer& operator=(SocketServer&&) = default;

    virtual ~SocketServer() = default;

    virtual std::optional<std::string> listen() = 0;

    virtual std::optional<std::string> close() = 0;

    virtual std::optional<std::string> start() = 0;

    std::shared_ptr<SocketServer> get_shared();

    int get_fd() const;

    void enable_thread_pool(std::size_t worker_num);

    void enable_event_loop(EventLoopType type = EventLoopType::SELECT);

    void set_logger(const utils::LoggerManager::Logger& logger);

    SocketType type() const;

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    virtual void on_read(CallBack handler);

    virtual void on_write(CallBack handler);

    virtual void on_error(CallBack handler);

    virtual void on_accept(CallBack handler);

    virtual std::optional<std::string> read(std::vector<uint8_t>& data, int client_fd) = 0;

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data, int client_fd) = 0;

protected:
    virtual void handle_connection(int client_fd) = 0;

    std::optional<std::string>
    get_peer_info(int fd, std::string& ip, std::string& service, addressResolver::address& info);

    int m_listen_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    CallBack m_accept_handler;
    CallBack m_on_read;
    CallBack m_on_write;
    CallBack m_on_error;

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