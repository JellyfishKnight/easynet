#include "tcp.hpp"

namespace net {

TcpClient::TcpClient(const std::string& ip, const std::string& service):
    SocketClient(ip, service, SocketType::TCP) {}

TcpServer::TcpServer(const std::string& ip, const std::string& service):
    SocketServer(ip, service, SocketType::TCP) {}

} // namespace net