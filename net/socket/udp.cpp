// #include "udp.hpp"
// #include "address_resolver.hpp"
// #include "remote_target.hpp"
// #include "socket_base.hpp"
// #include <algorithm>
// #include <cassert>
// #include <cstdint>
// #include <future>
// #include <memory>
// #include <optional>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <utility>
// #include <vector>

// namespace net {

// UdpClient::UdpClient(const std::string& ip, const std::string& service) {
//     m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
//     if (m_fd == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to create socket: {}", GET_ERROR_MSG());
//         }
//         throw std::system_error(errno, std::system_category(), "Failed to create socket");
//     }
//     m_addr_info = m_addr_resolver.resolve(ip, service);
//     m_ip = ip;
//     m_service = service;
//     m_logger_set = false;
//     m_status = SocketStatus::DISCONNECTED;
//     m_socket_type = SocketType::UDP;
// }

// UdpClient::~UdpClient() {
//     if (m_status == SocketStatus::CONNECTED) {
//         close();
//     }
// }

// std::optional<NetError> UdpClient::read(std::vector<uint8_t>& data) {
//     assert(data.size() > 0 && "Data buffer is empty");
//     ssize_t num_bytes = ::recvfrom(
//         m_fd,
//         data.data(),
//         data.size(),
//         0,
//         m_addr_info.get_address().m_addr,
//         &m_addr_info.get_address().m_len
//     );
//     if (num_bytes == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     if (num_bytes == 0) {
//         if (m_logger_set) {
//             NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
//         }
//         return "Connection reset by peer while reading";
//     }
//     data.resize(num_bytes);
//     return std::nullopt;
// }

// std::optional<NetError> UdpClient::write(const std::vector<uint8_t>& data) {
//     assert(data.size() > 0 && "Data buffer is empty");
//     ssize_t num_bytes =
//         ::sendto(m_fd, data.data(), data.size(), 0, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len);
//     if (num_bytes == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     if (num_bytes == 0) {
//         if (m_logger_set) {
//             NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
//         }
//         return "Connection reset by peer while writing";
//     }
//     return std::nullopt;
// }

// std::optional<NetError> UdpClient::close() {
//     if (::close(m_fd) == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to close socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     m_status = SocketStatus::DISCONNECTED;
//     return std::nullopt;
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] std::optional<NetError>
// UdpClient::connect() {
//     return std::nullopt;
// }

// std::shared_ptr<UdpClient> UdpClient::get_shared() {
//     return std::static_pointer_cast<UdpClient>(SocketClient::get_shared());
// }

// UdpServer::UdpServer(const std::string& ip, const std::string& service) {
//     struct ::addrinfo hints;
//     ::memset(&hints, 0, sizeof(hints));
//     hints.ai_socktype = SOCK_STREAM;
//     m_addr_info = m_addr_resolver.resolve(ip, service, &hints);
//     m_listen_fd = m_addr_info.create_socket();
//     m_ip = ip;
//     m_service = service;
//     m_logger_set = false;
//     m_epoll_enabled = false;
//     m_thread_pool = nullptr;
//     m_accept_handler = nullptr;
//     m_stop = true;
//     m_status = SocketStatus::DISCONNECTED;
//     m_socket_type = SocketType::UDP;
//     // bind
//     if (::bind(m_listen_fd, m_addr_info.get_address().m_addr, m_addr_info.get_address().m_len) == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to bind socket: {}", GET_ERROR_MSG());
//         }
//         throw std::system_error(errno, std::system_category(), "Failed to bind socket");
//     }
// }

// UdpServer::~UdpServer() {
//     if (m_status == SocketStatus::CONNECTED) {
//         close();
//     }
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] std::optional<NetError>
// UdpServer::listen() {
//     return std::nullopt;
// }

// std::optional<NetError> UdpServer::read(std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
//     addressResolver::address client_addr;
//     ssize_t num_bytes =
//         ::recvfrom(m_listen_fd, data.data(), data.size(), 0, &client_addr.m_addr, &client_addr.m_addr_len);
//     if (num_bytes == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     if (num_bytes == 0) {
//         if (m_logger_set) {
//             NET_LOG_WARN(m_logger, "Connection reset by peer while reading");
//         }
//         return "Connection reset by peer while reading";
//     }
//     if (remote == nullptr) {
//         remote = std::make_shared<RemoteTarget>();
//     }
//     remote->m_addr.m_addr = client_addr.m_addr;
//     remote->m_addr.m_addr_len = client_addr.m_addr_len;
//     remote->m_server_fd = m_listen_fd;
//     remote->m_status = SocketStatus::CONNECTED;
//     remote->m_server_ip = m_ip;
//     remote->m_server_service = m_service;
//     remote->m_client_ip = ::inet_ntoa(((struct sockaddr_in*)&client_addr.m_addr)->sin_addr);
//     remote->m_client_service = std::to_string(ntohs(((struct sockaddr_in*)&client_addr.m_addr)->sin_port));
//     return std::nullopt;
// }

// std::optional<NetError> UdpServer::write(const std::vector<uint8_t>& data, RemoteTarget::SharedPtr remote) {
//     assert(remote != nullptr && "RemoteTarget is nullptr");
//     ssize_t num_bytes =
//         ::sendto(m_listen_fd, data.data(), data.size(), 0, &remote->m_addr.m_addr, remote->m_addr.m_addr_len);
//     if (num_bytes == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     if (num_bytes == 0) {
//         if (m_logger_set) {
//             NET_LOG_WARN(m_logger, "Connection reset by peer while writing");
//         }
//         return "Connection reset by peer while writing";
//     }
//     return std::nullopt;
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] std::optional<NetError>
// UdpServer::read(std::vector<uint8_t>& data, RemoteTarget::ConstSharedPtr remote) {
//     return std::nullopt;
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] std::optional<NetError>
// UdpServer::write(const std::vector<uint8_t>& data, RemoteTarget::ConstSharedPtr remote) {
//     return std::nullopt;
// }

// std::optional<NetError> UdpServer::close() {
//     if (::close(m_listen_fd) == -1) {
//         if (m_logger_set) {
//             NET_LOG_ERROR(m_logger, "Failed to close socket: {}", GET_ERROR_MSG());
//         }
//         return GET_ERROR_MSG();
//     }
//     m_status = SocketStatus::DISCONNECTED;
//     return std::nullopt;
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] std::optional<NetError>
// UdpServer::start() {
//     m_stop = false;
//     if (m_epoll_enabled) {
//         m_accept_thread = std::thread([this]() {
//             while (!m_stop) {
//                 int num_events = ::epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), -1);
//                 if (num_events == -1) {
//                     if (m_logger_set) {
//                         NET_LOG_ERROR(m_logger, "Failed to wait for events: {}", GET_ERROR_MSG());
//                     }
//                     std::cerr << std::format("Failed to wait for events: {}\n", GET_ERROR_MSG());
//                     continue;
//                 }
//                 for (int i = 0; i < num_events; ++i) {
//                     if (m_events[i].events & EPOLLIN) {
//                         RemoteTarget::SharedPtr remote = std::make_shared<RemoteTarget>();
//                         std::vector<uint8_t> buffer(1024);
//                         auto err = this->read(buffer, remote);
//                         if (err.has_value()) {
//                             if (m_logger_set) {
//                                 NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", err.value());
//                             }
//                             std::cerr << std::format("Failed to read from socket: {}\n", err.value());
//                             continue;
//                         }
//                         m_connections.insert_or_assign({ remote->m_client_ip, remote->m_client_service }, remote);

//                         auto task = [b = std::move(buffer), remote, this]() {
//                             std::vector<uint8_t> res, req = b;
//                             m_message_handler(res, req, remote);
//                             if (res.empty()) {
//                                 return;
//                             }
//                             auto err = this->write(res, remote);
//                             if (err.has_value()) {
//                                 if (m_logger_set) {
//                                     NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", err.value());
//                                 }
//                                 std::cerr << std::format("Failed to write to socket: {}\n", err.value());
//                                 m_connections.erase({ remote->m_client_ip, remote->m_client_service });
//                             }
//                         };

//                         if (m_thread_pool) {
//                             this->m_thread_pool->submit(task);
//                         } else {
//                             auto unused_future = std::async(std::launch::async, task);
//                         }
//                     }
//                 }
//             }
//         });
//     } else {
//         m_accept_thread = std::thread([this]() {
//             while (!m_stop) {
//                 // receive is handled by main thread, and compute is handled by thread pool
//                 RemoteTarget::SharedPtr remote = std::make_shared<RemoteTarget>();
//                 std::vector<uint8_t> buffer(1024);
//                 auto err = this->read(buffer, remote);
//                 if (err.has_value()) {
//                     if (m_logger_set) {
//                         NET_LOG_ERROR(m_logger, "Failed to read from socket: {}", err.value());
//                     }
//                     std::cerr << std::format("Failed to read from socket: {}\n", err.value());
//                     continue;
//                 }
//                 m_connections.insert_or_assign({ remote->m_client_ip, remote->m_client_service }, remote);
//                 // handle func of udp server shall only been exeute once
//                 auto handle_func = [this, buffer_capture = std::move(buffer), remote]() {
//                     std::vector<uint8_t> res, req = buffer_capture;
//                     m_message_handler(res, req, remote);
//                     if (res.empty()) {
//                         return;
//                     }
//                     auto err = this->write(res, remote);
//                     if (err.has_value()) {
//                         if (m_logger_set) {
//                             NET_LOG_ERROR(m_logger, "Failed to write to socket: {}", err.value());
//                         }
//                         std::cerr << std::format("Failed to write to socket: {}\n", err.value());
//                         m_connections.erase({ remote->m_client_ip, remote->m_client_service });
//                     }
//                 };
//                 if (m_thread_pool) {
//                     m_thread_pool->submit(handle_func);
//                 } else {
//                     auto unused_future = std::async(std::launch::async, handle_func);
//                 }
//             }
//         });
//     }
//     return std::nullopt;
// }

// std::shared_ptr<UdpServer> UdpServer::get_shared() {
//     return std::static_pointer_cast<UdpServer>(SocketServer::get_shared());
// }

// [[deprecated("Udp doesn't need connection, this function will cause no effect")]] void
// UdpServer::on_start(std::function<void(RemoteTarget::ConstSharedPtr remote)> handler) {}

// void UdpServer::on_message(
//     std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&, RemoteTarget::ConstSharedPtr)> handler
// ) {
//     m_message_handler = std::move(handler);
// }

// } // namespace net