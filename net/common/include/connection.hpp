#pragma once

#include "address_resolver.hpp"
#include "parser.hpp"
#include "thread_pool.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace net {

enum class ConnectionStatus { CONNECTED, DISCONNECTED };

struct Connection: std::enable_shared_from_this<Connection> {
    using SharedPtr = std::shared_ptr<Connection>;

    int m_client_fd;
    int m_server_fd;

    addressResolver::address m_addr;
    ConnectionStatus m_status = ConnectionStatus::DISCONNECTED;
};

template<typename ResType, typename ReqType, typename ConnectionType, typename ParserType>
    requires std::is_base_of_v<Connection, ConnectionType>
    && std::is_base_of_v<BaseParser<ReqType, ResType>, ParserType>
class Server: std::enable_shared_from_this<Server<ResType, ReqType, ConnectionType, ParserType>> {
public:
    /**
     * @brief Construct a new Server object
     * @param ip ip address to bind to
     * @param service service to bind to
     */
    Server(const std::string& ip, const std::string& service): m_ip(ip), m_service(service) {}

    Server(const Server&) = delete;

    Server(Server&&) = default;

    Server& operator=(const Server&) = delete;

    Server& operator=(Server&&) = default;

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
                        "Failed to accept Connection: {}",
                        std::error_code(errno, std::system_category()).message()
                    ) << std::endl;
                    continue;
                }
                std::string client_ip =
                    ::inet_ntoa(((struct sockaddr_in*)&client_addr.m_addr)->sin_addr);
                std::string client_service =
                    std::to_string(ntohs(((struct sockaddr_in*)&client_addr.m_addr)->sin_port));
                auto& conn = m_Connections[client_ip][client_service];
                conn.m_client_fd = client_fd;
                conn.m_server_fd = m_lisen_fd;
                conn.m_addr = client_addr;
                if (m_thread_pool) {
                    m_thread_pool->submit([this, conn]() { handle_connection(); });
                } else {
                    std::thread([this]() { handle_connection(); }).detach();
                }
                conn.m_status = ConnectionStatus::CONNECTED;
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
                                "Failed to accept Connection: {}",
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
                        std::string client_ip =
                            ::inet_ntoa(((struct sockaddr_in*)&client_addr.m_addr)->sin_addr);
                        std::string client_service = std::to_string(
                            ntohs(((struct sockaddr_in*)&client_addr.m_addr)->sin_port)
                        );
                        auto& conn = m_Connections[client_ip][client_service];
                        conn.m_client_fd = client_fd;
                        conn.m_addr = client_addr;
                    } else {
                        handle_connection_epoll();
                    }
                }
            }
        });
        return true;
    }

    /**
     * @brief Add a handler for a Server, the old handler will be kept in threadpool
     * @param handler handler function
     */
    bool add_handler(std::function<ResType(const ReqType&)> handler) {
        m_default_handler = handler;
    }

    /**
     * @brief Destroy the Server object
     */
    ~Server() {
        stop();
        ::close(m_epoll_fd);
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

        for (auto& [ip, services]: m_Connections) {
            for (auto& [service, conn]: services) {
                ::close(conn.m_client_fd);
            }
        }

        m_Connections.clear();

        if (m_thread_pool) {
            m_thread_pool->stop();
        }
    }

    void stop(const std::string& ip, const std::string& service) {
        if (m_Connections.contains(ip)) {
            if (m_Connections[ip].contains(service)) {
                auto& Connection = m_Connections[ip][service];
                ::close(Connection.m_client_fd);
                m_Connections[ip].erase(service);
            }
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

protected:
    virtual void write_res(const ResType& res, const Connection& fd) = 0;

    virtual void read_req(ReqType& req, const Connection& fd) = 0;

    virtual void handle_connection(const Connection& conn) = 0;

    virtual void handle_connection_epoll() = 0;

    std::string m_ip;
    std::string m_service;
    int m_lisen_fd;
    int m_epoll_fd;

    bool m_stop = false;
    bool m_epoll = false;

    std::function<ResType(const ReqType&)> m_default_handler;

    std::thread m_accept_thread;

    utils::ThreadPool::SharedPtr m_thread_pool;

    // first key : ip, second key : port
    std::unordered_map<std::string, std::unordered_map<std::string, ConnectionType>> m_Connections;

    std::shared_ptr<ParserType> m_parser;
};

template<typename ResType, typename ReqType, typename ParserType>
    requires std::is_base_of_v<BaseParser<ReqType, ResType>, ParserType>
class Client: std::enable_shared_from_this<Client<ResType, ReqType, ParserType>> {
public:
    Client(const std::string& ip, const std::string& service): m_ip(ip), m_service(service) {}

    Client(const Client&) = delete;

    Client(Client&&) = default;

    Client& operator=(const Client&) = delete;

    Client& operator=(Client&&) = default;

    void connect() {
        m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_fd == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create socket");
        }

        addressResolver resolver;
        m_addr = resolver.resolve(m_ip, m_service).get_address();
        if (::connect(m_fd, m_addr.m_addr, m_addr.m_len) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to connect to server");
        }
    }

    void close() {
        if (::close(m_fd) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to close socket");
        }
    }

    ResType read_res() {
        std::vector<uint8_t> buffer(1024);
        ssize_t num_bytes = ::recv(m_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to receive data");
        }
        if (num_bytes == 0) {
            throw std::system_error(errno, std::system_category(), "Connection closed");
        }
        ResType res;
        m_parser->read_req(buffer);
        return res;
    }

    void write_req(const ReqType& req) {
        auto buffer = m_parser->write_req(req);

        if (::send(m_fd, buffer.data(), buffer.size(), 0) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
    }

private:
    std::string m_ip;
    std::string m_service;
    int m_fd;
    addressResolver::address_ref m_addr;

    std::shared_ptr<ParserType> m_parser;

    ConnectionStatus m_status = ConnectionStatus::DISCONNECTED;
};

} // namespace net
