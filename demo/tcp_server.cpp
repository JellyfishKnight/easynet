#include "socket.hpp"
#include <cstdlib>
#include <memory>

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
        server->add_handler([server](const std::vector<uint8_t>& req) {
            std::string req_str { req.begin(), req.end() };
            std::cout << "Received request: " << req_str << std::endl;
            return std::vector<uint8_t> { req_str.begin(), req_str.end() };
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
        std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            server->stop();
            break;
        }
    };

    std::cout << "Server stopped" << std::endl;

    return 0;
}