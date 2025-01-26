#include "socket_base.hpp"
#include "address_resolver.hpp"
#include "connection.hpp"
#include "logger.hpp"
#include <cassert>
#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace net {

int SocketClient::get_fd() const {
    return m_fd;
}

SocketType SocketClient::type() const {
    return m_socket_type;
}

void SocketClient::set_logger(const utils::LoggerManager::Logger& logger) {
    m_logger = logger;
    m_logger_set = true;
}

std::string SocketClient::get_error_msg() {
    // read error from system cateogry
    return std::system_category().message(errno);
}

std::string SocketClient::get_ip() const {
    return m_ip;
}

std::string SocketClient::get_service() const {
    return m_service;
}

ConnectionStatus SocketClient::status() const {
    return m_status;
}

SocketType SocketServer::type() const {
    return m_socket_type;
}

int SocketServer::get_fd() const {
    return m_listen_fd;
}

void SocketServer::set_logger(const utils::LoggerManager::Logger& logger) {
    m_logger = logger;
    m_logger_set = true;
}

std::string SocketServer::get_error_msg() {
    // read error from system cateogry
    return std::system_category().message(errno);
}

void SocketServer::enable_thread_pool(std::size_t worker_num) {
    assert(worker_num > 0 && "Worker number should be greater than 0");
    assert(m_thread_pool == nullptr && "Thread pool is already enabled");
    assert(
        m_status == ConnectionStatus::DISCONNECTED
        || m_status == ConnectionStatus::LISTENING && "Server is already connected"
    );
    m_thread_pool = std::make_shared<utils::ThreadPool>(worker_num);
}

std::optional<std::string> SocketServer::enable_epoll(std::size_t event_num) {
    assert(
        m_status == ConnectionStatus::DISCONNECTED
        || m_status == ConnectionStatus::LISTENING && "Server is already connected"
    );
    assert(event_num > 0 && "Event number should be greater than 0");
    m_epoll_fd = ::epoll_create1(0);
    if (m_epoll_fd == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to create epoll: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_events.resize(event_num);
    m_epoll_enabled = true;
    return std::nullopt;
}

void SocketServer::add_handler(std::function<void(
                                   std::vector<uint8_t>& res,
                                   std::vector<uint8_t>& req,
                                   const Connection::SharedPtr conn
                               )> handler) {
    m_default_handler = handler;
}

std::optional<std::string> SocketServer::get_peer_info(
    int fd,
    std::string& ip,
    std::string& service,
    addressResolver::address& info
) {
    if (::getpeername(fd, &info.m_addr, &info.m_addr_len) == 0) {
        ip = ::inet_ntoa(((struct sockaddr_in*)&info.m_addr)->sin_addr);
        service = std::to_string(ntohs(((struct sockaddr_in*)&info.m_addr)->sin_port));
        return std::nullopt;
    }
    return std::format("Get peer info failed: {}", get_error_msg());
}

std::string SocketServer::get_ip() const {
    return m_ip;
}

std::string SocketServer::get_service() const {
    return m_service;
}

ConnectionStatus SocketServer::status() const {
    return m_status;
}

} // namespace net