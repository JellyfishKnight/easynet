#include "event_loop.hpp"
#include "remote_target.hpp"
#include "socket.hpp"
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string_view>
#include <vector>

int main() {
    std::shared_ptr<net::TcpServer> server;
    try {
        server = std::make_shared<net::TcpServer>("127.0.0.1", "8080");
    } catch (std::exception const& e) {
        std::cerr << "Failed to create server: " << e.what() << std::endl;
        return 1;
    }

    auto err = server->listen();
    if (err.has_value()) {
        std::cerr << "Failed to listen: " << err.value() << std::endl;
        return 1;
    }

    try {
        server->enable_thread_pool(96);
        auto err = server->enable_event_loop();
        if (err.has_value()) {
            std::cerr << "Failed to enable event loop: " << err.value() << std::endl;
            return 1;
        }

        auto handler = [server](const net::RemoteTarget& conn) {
            std::vector<uint8_t> req;
            auto err = server->read(req, conn);
            if (err.has_value()) {
                std::cerr << std::format("Failed to read from socket {} : {}\n", conn.m_client_fd, err.value());
                return;
            }
            std::vector<uint8_t> res { req.begin(), req.end() };
            auto err_write = server->write(res, conn);
            if (err_write.has_value()) {
                std::cerr << std::format("Failed to write to socket {} : {}\n", conn.m_client_fd, err.value());
                return;
            }
        };
        server->on_read(handler);
        server->on_accept(handler);
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
        std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            server->close();
            break;
        }
    };

    std::cout << "Server stopped" << std::endl;

    return 0;
}