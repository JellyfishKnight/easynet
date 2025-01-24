#include "udp.hpp"
#include <sys/socket.h>

namespace net {

UdpClient::UdpClient(const std::string& ip, const std::string& service, SocketType type):
    SocketClient(ip, service, type) {}

std::optional<std::string> UdpClient::read(std::vector<uint8_t>& data) {
    ::socklen_t len = m_addr_info.get_address().m_len;
    ssize_t num_bytes =
        ::recvfrom(m_fd, data.data(), data.size(), 0, m_addr_info.get_address().m_addr, &len);
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

std::optional<std::string> UdpClient::write(const std::vector<uint8_t>& data) {
    ssize_t num_bytes = ::sendto(
        m_fd,
        data.data(),
        data.size(),
        0,
        m_addr_info.get_address().m_addr,
        m_addr_info.get_address().m_len
    );
    if (num_bytes == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    if (num_bytes == 0) {
        if (m_logger_set) {
            NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
        }
        return "Connection reset by peer while writing";
    }
    return std::nullopt;
}

std::optional<std::string> UdpClient::close() {
    if (::close(m_fd) == -1) {
        if (m_logger_set) {
            NET_LOG_ERROR(m_logger, "Failed to close socket: {}", get_error_msg());
        }
        return get_error_msg();
    }
    m_status = ConnectionStatus::DISCONNECTED;
    return std::nullopt;
}

[[deprecated("Udp doesn't need connection, this function will cause no effect"
)]] std::optional<std::string>
UdpClient::connect() {
    return std::nullopt;
}

} // namespace net