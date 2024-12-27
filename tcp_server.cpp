#include "common.hpp"
#include "tcp.hpp"
#include <iostream>
#include "http_server.hpp"

int main() {
    // net::TcpServer server("127.0.0.1", 15238);
    // server.listen(10);

    net::HttpServer server("localhost", 2789);

    auto server_thread = std::thread([&server] {
        server.start();
    });

    server_thread.join();
    return 0;
}