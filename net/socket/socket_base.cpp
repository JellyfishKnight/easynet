#include "socket_base.hpp"
#include "address_resolver.hpp"
#include "defines.hpp"
#include "event_loop.hpp"
#include "logger.hpp"
#include "remote_target.hpp"
#include <cassert>
#include <cstddef>
#include <fcntl.h>
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
#include <unordered_map>

namespace net {

/********************************************************************************/
/********************************************************************************/
/*********************************Socket Client**********************************/
/********************************************************************************/
/********************************************************************************/

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

std::string SocketClient::get_ip() const {
    return m_ip;
}

std::string SocketClient::get_service() const {
    return m_service;
}

SocketStatus SocketClient::status() const {
    return m_status;
}

std::shared_ptr<SocketClient> SocketClient::get_shared() {
    return shared_from_this();
}

std::optional<std::string> SocketClient::set_non_blocking_socket(int fd) {
    int flag = ::fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        return get_error_msg();
    }
    if (::fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        return get_error_msg();
    }
    return std::nullopt;
}

/********************************************************************************/
/********************************************************************************/
/*********************************Socket Server**********************************/
/********************************************************************************/
/********************************************************************************/

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

void SocketServer::enable_thread_pool(std::size_t worker_num) {
    assert(worker_num > 0 && "Worker number should be greater than 0");
    assert(m_thread_pool == nullptr && "Thread pool is already enabled");
    assert(
        m_status == SocketStatus::DISCONNECTED || m_status == SocketStatus::LISTENING && "Server is already connected"
    );
    m_thread_pool = std::make_shared<utils::ThreadPool>(worker_num);
}

std::optional<std::string> SocketServer::enable_event_loop(EventLoopType type, int time_out) {
    assert(
        m_status == SocketStatus::DISCONNECTED || m_status == SocketStatus::LISTENING && "Server is already connected"
    );
    if (type == EventLoopType::SELECT) {
        std::cerr
            << "Select way is not stable for it can't handle more than 1024 connections even if the value of socket_fd\
             is more than 1024\n";
        m_event_loop = std::make_shared<SelectEventLoop>(time_out);
    } else if (type == EventLoopType::EPOLL) {
        m_event_loop = std::make_shared<EpollEventLoop>(time_out);
    } else if (type == EventLoopType::POLL) {
        m_event_loop = std::make_shared<PollEventLoop>(time_out);
    } else {
        return "Invalid event loop type";
    }
    return std::nullopt;
}

void SocketServer::on_start(std::function<void(RemoteTarget::SharedPtr)> handler) {
    m_accept_handler = handler;
}

void SocketServer::on_read(CallBack handler) {
    m_on_read = handler;
}

void SocketServer::on_write(CallBack handler) {
    m_on_write = handler;
}

void SocketServer::on_error(CallBack handler) {
    m_on_error = handler;
}

void SocketServer::on_accept(CallBack handler) {
    m_on_accept = handler;
}

std::optional<std::string>
SocketServer::get_peer_info(int fd, std::string& ip, std::string& service, addressResolver::address& info) {
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

SocketStatus SocketServer::status() const {
    return m_status;
}

std::shared_ptr<SocketServer> SocketServer::get_shared() {
    return shared_from_this();
}

std::optional<std::string> SocketServer::set_non_blocking_socket(int fd) {
    int flag = ::fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        return get_error_msg();
    }
    if (::fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        return get_error_msg();
    }
    return std::nullopt;
}

} // namespace net