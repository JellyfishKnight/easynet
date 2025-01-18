#pragma once

#include "address_resolver.hpp"
#include "thread_pool.hpp"
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <unordered_map>

namespace net {

enum class ConnectionStatus { CONNECTED, DISCONNECTED };

template<typename ResType, typename ReqType>
struct Connection: std::enable_shared_from_this<Connection<ResType, ReqType>> {
    int m_fd;
    addressResolver::address m_addr;
    std::function<ResType(const ReqType&)> m_handler;
    ConnectionStatus m_status = ConnectionStatus::DISCONNECTED;

    virtual std::function<ResType(const ReqType&)> get_handler() {
        return m_handler;
    }
};

template<typename ResType, typename ReqType>
class Server: std::enable_shared_from_this<Server<ResType, ReqType>> {
public:
    /**
     * @brief Construct a new Server object
     * @param ip ip address to bind to
     * @param service service to bind to
     */
    Server(const std::string& ip, const std::string& service): m_ip(ip), m_service(service) {}

    /**
     * @brief Listen on the socket
     * @throw std::system_error if failed to listen on socket
     */
    void listen() {
        m_lisen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_lisen_fd == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create socket");
        }

        addressResolver resolver;
        auto addr_info = resolver.resolve(m_ip, m_service);
        if (::bind(m_lisen_fd, addr_info.get_address().m_addr, addr_info.get_address().m_len) == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to bind socket");
        }

        if (::listen(m_lisen_fd, 10) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
        }
    }

    /*
     * @brief Start the server
    */
    bool start() {
        m_stop = false;
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                addressResolver::address client_addr;
                socklen_t len;
                int client_fd = ::accept(m_lisen_fd, &client_addr.m_addr, &len);
                if (client_fd == -1) {
                    std::cerr << std::format(
                        "Failed to accept connection: {}",
                        std::error_code(errno, std::system_category()).message()
                    ) << std::endl;
                    continue;
                }
                if (m_connections.contains(m_ip)) {
                    if (m_connections[m_ip].contains(m_service)) {
                        auto& connection = m_connections[m_ip][m_service];
                        connection.m_fd = client_fd;
                        connection.m_addr = client_addr;
                        ///TODO: run handler

                        connection.m_status = ConnectionStatus::CONNECTED;
                    } else {
                        m_connections[m_ip][m_service] = { client_fd, client_addr, nullptr };
                    }
                } else {
                    m_connections[m_ip][m_service] = { client_fd, client_addr, nullptr };
                }
            }
        });
        return true;
    }

    bool start_epoll() {
        if (!m_epoll) {
            std::cerr << "Epoll is not enabled, please enable epoll first" << std::endl;
            return false;
        }
        m_stop = false;
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                struct ::epoll_event events[10];
                int num_events = ::epoll_wait(m_epoll_fd, events, 10, -1);
                if (num_events == -1) {
                    std::cerr << std::format(
                        "Failed to wait for events: {}",
                        std::error_code(errno, std::system_category()).message()
                    ) << std::endl;
                    continue;
                }
                for (int i = 0; i < num_events; ++i) {
                    if (events[i].data.fd == m_lisen_fd) {
                        addressResolver::address client_addr;
                        socklen_t len;
                        int client_fd = ::accept(m_lisen_fd, &client_addr.m_addr, &len);
                        if (client_fd == -1) {
                            std::cerr << std::format(
                                "Failed to accept connection: {}",
                                std::error_code(errno, std::system_category()).message()
                            ) << std::endl;
                            continue;
                        }
                        struct ::epoll_event event;
                        event.events = EPOLLIN;
                        event.data.fd = client_fd;
                        if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                            std::cerr << std::format(
                                "Failed to add client socket to epoll: {}",
                                std::error_code(errno, std::system_category()).message()
                            ) << std::endl;
                            continue;
                        }
                        if (m_connections.contains(m_ip)) {
                            if (m_connections[m_ip].contains(m_service)) {
                                auto& connection = m_connections[m_ip][m_service];
                                connection.m_fd = client_fd;
                                connection.m_addr = client_addr;
                            }
                        } else {
                            m_connections[m_ip][m_service] = { client_fd, client_addr, nullptr };
                        }
                    } else {
                        ///TODO: finish
                        auto& connection = m_connections[m_ip][m_service];
                        char buffer[1024];
                        int num_bytes = ::recv(events[i].data.fd, buffer, 1024, 0);
                        if (num_bytes == -1) {
                            std::cerr << std::format(
                                "Failed to receive data: {}",
                                std::error_code(errno, std::system_category()).message()
                            ) << std::endl;
                            continue;
                        }
                        ReqType req;
                        ResType res;
                        connection.m_handler(req);
                    }
                }
            }
        });
        return true;
    }

    /**
     * @brief Add a handler for a connection
     * @param ip ip address of the connection
     * @param service service of the connection
     * @param handler handler function
     * @return true if handler was added successfully
     */
    bool add_handler(
        const std::string& ip,
        const std::string& service,
        std::function<ResType(const ReqType&)> handler
    ) {
        if (m_connections.contains(ip)) {
            if (m_connections[ip].contains(service)) {
                auto& conn = m_connections[ip][service];
                conn.handler = handler;
                ///TODO: run handler for existing connections
                if (conn.m_status == ConnectionStatus::CONNECTED) {
                } else {
                    conn.m_status = ConnectionStatus::CONNECTED;
                }
                return true;
            }
        }
        m_connections[ip][service].m_handler = handler;
    }

    /**
     * @brief Destroy the Server object
     */
    ~Server() {
        ::close(m_lisen_fd);
    }

    /**
     * @brief Enable thread pool
     * @param num_threads number of threads in the pool
     */
    void enable_thread_pool(std::size_t num_threads) {
        m_thread_pool = std::make_shared<utils::ThreadPool>(num_threads);
    }

    /**
     * @brief Disable thread pool
     */
    void disable_thread_pool() {
        m_thread_pool->stop();
        while (m_thread_pool->is_running())
            ;
        m_thread_pool.reset();
    }

    /**
     * @brief Set the thread pool size
     * @param num_threads number of threads in the pool
     */
    void set_thread_pool_size(std::size_t num_threads) {
        if (m_thread_pool) {
            if (m_thread_pool->worker_num() == num_threads) {
                return;
            }
            m_thread_pool->resize_worker(num_threads);
        }
    }

    /**
     * @brief Stop the server
     */
    void stop() {
        m_stop = true;
        m_accept_thread.join();

        for (auto& [ip, services]: m_connections) {
            for (auto& [service, conn]: services) {
                ::close(conn.m_fd);
            }
        }

        m_connections.clear();

        if (m_thread_pool) {
            m_thread_pool->stop();
        }
    }

    void enable_epoll() {
        // set none blocking
        if (::fcntl(m_lisen_fd, F_GETFL, 0) == -1 || ::fcntl(m_lisen_fd, F_SETFL, O_NONBLOCK) == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to get socket flags");
        }

        m_epoll_fd = ::epoll_create1(0);
        if (m_epoll_fd == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create epoll");
        }
        struct ::epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = m_lisen_fd;
        if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_lisen_fd, &event) == -1) {
            throw std::system_error(
                errno,
                std::system_category(),
                "Failed to add listen socket to epoll"
            );
        }
    }

private:
    std::string m_ip;
    std::string m_service;
    int m_lisen_fd;
    int m_epoll_fd;

    bool m_stop = false;
    bool m_epoll = false;

    std::thread m_accept_thread;

    utils::ThreadPool::SharedPtr m_thread_pool;

    // first key : ip, second key : port
    std::unordered_map<std::string, std::unordered_map<std::string, Connection<ReqType, ResType>>>
        m_connections;
};

template<typename ResType, typename ReqType>
class Client: std::enable_shared_from_this<Client<ResType, ReqType>> {};

} // namespace net
