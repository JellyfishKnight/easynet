#pragma once

#include "address_resolver.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace net {

template<typename ResType, typename ReqType>
class Server: std::enable_shared_from_this<Server<ResType, ReqType>> {
public:
    struct Connection {
        int m_fd;
        addressResolver::address m_addr;
    };

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

        m_epoll_fd = ::epoll_create1(0);
        if (m_epoll_fd == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to create epoll");
        }

        struct epoll_event event;
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

    // first key : ip, second key : port
    std::unordered_map<std::string, std::unordered_map<std::string, Connection>> m_connections;
};

template<typename ResType, typename ReqType>
class Client: std::enable_shared_from_this<Client<ResType, ReqType>> {};

} // namespace net
