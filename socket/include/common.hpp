#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <functional>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace net {

enum class SocketStatus : int { CLOSED = 0, CONNECTED, LISTENING, ACCEPTED };

struct ClientConnection {
    SocketStatus status = SocketStatus::CLOSED;
    std::string client_ip;

    int client_port;

    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;

    ClientConnection() = default;

    ClientConnection(const ClientConnection&) = delete;

    ClientConnection& operator=(const ClientConnection&) = delete;

    ClientConnection(ClientConnection&& other) noexcept:
        status(other.status),
        client_ip(std::move(other.client_ip)),
        client_port(other.client_port),
        sockfd(other.sockfd),
        servaddr(other.servaddr),
        cliaddr(other.cliaddr) {}

    ClientConnection& operator=(ClientConnection&& other) noexcept {
        status = other.status;
        client_ip = std::move(other.client_ip);
        client_port = other.client_port;
        sockfd = other.sockfd;
        servaddr = other.servaddr;
        cliaddr = other.cliaddr;
        return *this;
    }

    ~ClientConnection() {
        if (status != SocketStatus::CLOSED) {
            ::close(sockfd);
        }
    }
};


// template<typename Server, int frequency = 0, int waiting_queue_size = 1>
// void start_server(Server& server, std::function<void(Server&)> callback) {
//     try {
//         server.listen(waiting_queue_size);
//         if (frequency == 0) {
//             std::thread([&server, callback]() {
//                 while (true) {
//                     server.accept();
//                     callback(server);
//                 }
//             }).detach();
//         } else {
//             std::thread([&server, callback]() {
//                 while (true) {
//                     server.accept();
//                     callback(server);
//                     std::this_thread::sleep_for(std::chrono::milliseconds(frequency));
//                 }
//             }).detach();
//         }
//     } catch (const std::exception& e) {
//         throw e;
//     }
// }

} // namespace net
