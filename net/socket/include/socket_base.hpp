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
#include <unordered_map>
#include <vector>

namespace net {

enum class SocketType : uint8_t {
    TCP = 1,
    UDP,
    RAW,
};

class SocketClient: std::enable_shared_from_this<SocketClient> {
public:
    SocketClient() = default;

    SocketClient(const SocketClient&) = delete;

    SocketClient& operator=(const SocketClient&) = delete;

    SocketClient(SocketClient&&) = default;

    SocketClient& operator=(SocketClient&&) = default;

    virtual ~SocketClient() = default;

    virtual std::optional<std::string> connect() = 0;

    virtual std::optional<std::string> close() = 0;

    int get_fd() const;

    SocketType type() const;

    void set_logger(const utils::LoggerManager::Logger& logger);

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

    virtual std::optional<std::string> read(std::vector<uint8_t>& data) = 0;

    virtual std::optional<std::string> write(const std::vector<uint8_t>& data) = 0;

protected:
    std::string get_error_msg();

    int m_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    ConnectionStatus m_status;
    SocketType m_socket_type;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

class SocketServer: std::enable_shared_from_this<SocketServer> {
public:
    SocketServer() = default;

    SocketServer(const SocketServer&) = delete;

    SocketServer& operator=(const SocketServer&) = delete;

    SocketServer(SocketServer&&) = default;

    SocketServer& operator=(SocketServer&&) = default;

    virtual ~SocketServer() = default;

    virtual std::optional<std::string> listen();

    virtual std::optional<std::string> close();

    virtual std::optional<std::string> start();

    int get_fd() const;

    void enable_thread_pool(std::size_t worker_num);

    std::optional<std::string> enable_epoll(std::size_t event_num);

    void set_logger(const utils::LoggerManager::Logger& logger);

    SocketType type() const;

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

    void
    add_handler(std::function<
                void(std::vector<uint8_t>& res, std::vector<uint8_t>& req, const Connection& conn)>
                    handler);

protected:
    virtual std::optional<std::string> read(std::vector<uint8_t>& data, Connection::SharedPtr conn);

    virtual std::optional<std::string>
    write(const std::vector<uint8_t>& data, Connection::SharedPtr conn);

    virtual void handle_connection(Connection::SharedPtr conn);

    std::string get_error_msg();

    std::optional<std::string> get_peer_info(
        int fd,
        std::string& ip,
        std::string& service,
        addressResolver::address_info& info
    );

    int m_listen_fd;
    addressResolver m_addr_resolver;
    addressResolver::address_info m_addr_info;
    std::string m_ip;
    std::string m_service;

    std::function<
        void(std::vector<uint8_t>& res, std::vector<uint8_t>& req, const Connection& conn)>
        m_default_handler;

    utils::ThreadPool::SharedPtr m_thread_pool;
    std::vector<struct ::epoll_event> m_events;
    int m_epoll_fd;

    std::unordered_map<ConnectionKey, Connection::SharedPtr> m_connections;
    bool m_stop;
    bool m_epoll_enabled;

    std::thread m_accept_thread;

    ConnectionStatus m_status;
    SocketType m_socket_type;

    bool m_logger_set;
    utils::LoggerManager::Logger m_logger;
};

} // namespace net