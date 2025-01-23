#pragma once

#include "address_resolver.hpp"
#include "connection.hpp"
#include "logger.hpp"
#include "thread_pool.hpp"
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>

namespace net {

class SocketClient: std::enable_shared_from_this<SocketClient> {
public:
    SocketClient(const std::string& ip, const std::string& service);

    SocketClient(const SocketClient&) = delete;

    SocketClient& operator=(const SocketClient&) = delete;

    SocketClient(SocketClient&&) = default;

    SocketClient& operator=(SocketClient&&) = default;

    virtual ~SocketClient();

    virtual std::optional<std::string> connect();

    virtual std::optional<std::string> read(std::vector<uint8_t>& data);

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data);

    virtual std::optional<std::string> close();

    int get_fd() const;

    void set_logger(const utils::LoggerManager::Logger& logger);

protected:
    std::string get_error_msg();

    int m_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    ConnectionStatus m_status;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

class SocketServer: std::enable_shared_from_this<SocketServer> {
public:
    SocketServer(const std::string& ip, const std::string& service);

    SocketServer(const SocketServer&) = delete;

    SocketServer& operator=(const SocketServer&) = delete;

    SocketServer(SocketServer&&) = default;

    SocketServer& operator=(SocketServer&&) = default;

    virtual ~SocketServer() = default;

    virtual std::optional<std::string> listen() = 0;

    virtual std::optional<std::string> read(std::vector<uint8_t>& data) = 0;

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data) = 0;

    virtual std::optional<std::string> close() = 0;

    int get_fd() const;

    virtual std::optional<std::string> start();

    void enable_thread_pool(std::size_t worker_num);

    void enable_epoll(std::size_t event_num);

protected:
    std::string get_error_msg();

    int m_listen_fd;
    addressResolver::address m_addr;

    std::function<void(std::vector<uint8_t>&, const std::vector<uint8_t>&)> m_default_handler;

    utils::ThreadPool::SharedPtr m_thread_pool;
    std::vector<struct ::epoll_event> m_events;
    int m_epoll_fd;

    bool m_stop;
    bool m_epoll_enabled;

    std::thread m_accept_thread;

    ConnectionStatus m_status;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

} // namespace net