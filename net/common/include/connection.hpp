#pragma once

#include "address_resolver.hpp"
#include "thread_pool.hpp"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>

namespace net {

template<typename ResType, typename ReqType>
class Server: std::enable_shared_from_this<Server<ResType, ReqType>> {
public:
    enum class ConnectionStatus { CONNECTED, DISCONNECTED };

    struct Connection {
        int m_fd;
        addressResolver::address m_addr;
        std::function<ResType(const ReqType&)> m_handler;
        ConnectionStatus m_status = ConnectionStatus::DISCONNECTED;
    };

    /**
     * @brief Construct a new Server object
     * @param ip ip address to bind to
     * @param service service to bind to
     * @throw std::system_error if failed to create socket, bind or listen
     */
    Server(const std::string& ip, const std::string& service): m_ip(ip), m_service(service) {
        m_lisen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_lisen_fd == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create socket");
        }

        addressResolver resolver;
        auto addr_info = resolver.resolve(ip, service);
        if (::bind(m_lisen_fd, addr_info.get_address().m_addr, addr_info.get_address().m_len) == -1)
        {
            throw std::system_error(errno, std::system_category(), "Failed to bind socket");
        }

        if (::listen(m_lisen_fd, 10) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
        }
    }

    void start() {
        m_stop = false;
        m_accept_thread = std::thread([this]() {
            while (!m_stop) {
                addressResolver::address client_addr;
                socklen_t len;
                int client_fd = ::accept(m_lisen_fd, &client_addr.m_addr, &len);
                if (client_fd == -1) {
                    throw std::system_error(
                        errno,
                        std::system_category(),
                        "Failed to accept connection"
                    );
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

    void enable_thread_pool(std::size_t num_threads) {
        m_thread_pool = std::make_shared<utils::ThreadPool>(num_threads);
    }

    void disable_thread_pool() {
        m_thread_pool->stop();
        while (m_thread_pool->is_running())
            ;
        m_thread_pool.reset();
    }

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

    // /**
    //  * @brief Start the server
    //  * @throw std::system_error if failed to create epoll
    //  */
    // void enable_epoll() {
    //     m_epoll_fd = ::epoll_create1(0);
    //     if (m_epoll_fd == -1) {
    //         throw std::system_error(errno, std::system_category(), "Failed to create epoll");
    //     }
    //     struct epoll_event event;
    //     event.events = EPOLLIN;
    //     event.data.fd = m_lisen_fd;
    //     if (::epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_lisen_fd, &event) == -1) {
    //         throw std::system_error(
    //             errno,
    //             std::system_category(),
    //             "Failed to add listen socket to epoll"
    //         );
    //     }
    // }

private:
    std::string m_ip;
    std::string m_service;
    int m_lisen_fd;
    int m_epoll_fd;

    bool m_stop = false;

    std::thread m_accept_thread;

    utils::ThreadPool::SharedPtr m_thread_pool;

    // first key : ip, second key : port
    std::unordered_map<std::string, std::unordered_map<std::string, Connection>> m_connections;
};

template<typename ResType, typename ReqType>
class Client: std::enable_shared_from_this<Client<ResType, ReqType>> {};

} // namespace net
