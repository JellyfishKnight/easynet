#include "socket.hpp"
#include "connection.hpp"
#include "logger.hpp"
#include <format>
#include <optional>
#include <sys/types.h>
#include <system_error>

namespace net {

SocketClient::SocketClient(const std::string& ip, const std::string& service) {
    m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to create socket: {}", get_error_msg());
        }
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }
    m_addr_info = m_addr_resolver.resolve(ip, service);
    m_ip = ip;
    m_service = service;
    m_logger_set = false;
    m_status = ConnectionStatus::DISCONNECTED;
}

SocketClient::~SocketClient() {
    if (m_status == ConnectionStatus::CONNECTED) {
        close();
    }
}

int SocketClient::get_fd() const {
    return m_fd;
}

void SocketClient::set_logger(const utils::LoggerManager::Logger& logger) {
    m_logger = logger;
    m_logger_set = true;
}

std::string SocketClient::get_error_msg() {
    // read error from system cateogry
    return std::format("{}", std::system_category().message(errno));
}

std::optional<std::string> SocketClient::connect() {
    if (::connect(m_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to connect to server: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::CONNECTED;
    return std::nullopt;
}

std::optional<std::string> SocketClient::read(std::vector<uint8_t>& data) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Client is not connected";
    }
    ssize_t num_bytes = ::recv(m_fd, data.data(), data.size(), 0);
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while reading";
    }
    data.resize(num_bytes);
    return std::nullopt;
}

std::optional<std::string> SocketClient::write(const std::vector<uint8_t>& data) {
    if (m_status != ConnectionStatus::CONNECTED) {
        return "Client is not connected";
    }
    ssize_t num_bytes = ::send(m_fd, data.data(), data.size(), 0);
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string> SocketClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

SocketServer::SocketServer(const std::string& ip, const std::string& service) {}

} // namespace net