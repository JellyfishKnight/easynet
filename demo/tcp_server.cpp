#include "socket.hpp"

int main() {
    net::TcpServer server("127.0.0.1", "8080");

    server.listen();

    server.enable_thread_pool(10);
    server.add_handler([](const std::vector<uint8_t>& req) {
        std::string req_str { req.begin(), req.end() };
        std::cout << "Received request: " << req_str << std::endl;
        return std::vector<uint8_t> { req.rbegin(), req.rend() };
    });

    server.start();

    return 0;
}