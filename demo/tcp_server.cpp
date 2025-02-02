#include "socket.hpp"
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

int main() {
    std::shared_ptr<net::TcpServer> server;
    try {
        server = std::make_shared<net::TcpServer>("127.0.0.1", "8080");
    } catch (std::system_error const& e) {
        std::cerr << "Failed to create server: " << e.what() << std::endl;
        return 1;
    }

    try {
        server->listen();
    } catch (std::system_error const& e) {
        std::cerr << "Failed to listen on socket: " << e.what() << std::endl;
        return 1;
    }

    try {
        server->enable_thread_pool(10);
        // server->enable_epoll(20);
        server->on_accept([server](net::Connection::ConstSharedPtr conn) {
            while (server->status() == net::ConnectionStatus::CONNECTED) {
                std::vector<uint8_t> req(1024);
                auto err = server->read(req, conn);
                if (err.has_value()) {
                    std::cerr << "Failed to read from socket: " << err.value() << std::endl;
                    continue;
                }
                if (req.empty()) {
                    continue;
                }
                std::vector<uint8_t> res { req.begin(), req.end() };
                auto err_write = server->write(res, conn);
                if (err_write.has_value()) {
                    std::cerr << "Failed to write to socket: " << err_write.value() << std::endl;
                    continue;
                }
            }
        });
    } catch (std::system_error const& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        return 1;
    }

    try {
        server->start();
    } catch (std::system_error const& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        return 1;
    }
    while (true) {
        std::string input;
        std::cin >> input;
        // std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            server->close();
            break;
        }
    };

    std::cout << "Server stopped" << std::endl;

    return 0;
}