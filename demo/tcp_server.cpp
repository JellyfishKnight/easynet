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
        server->add_handler([](std::vector<uint8_t>& res,
                               std::vector<uint8_t>& req,
                               net::Connection::ConstSharedPtr conn) {
            std::string str { reinterpret_cast<char*>(req.data()), req.size() };
            std::cout << str << std::endl;
            res = req;
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