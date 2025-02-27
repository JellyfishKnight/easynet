#include "event_loop.hpp"
#include "remote_target.hpp"
#include "socket.hpp"
#include "timer.hpp"
#include <cstdint>
#include <cstdio>
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
        std::cerr << "Failed to listen: " << err.value().msg << std::endl;
        return 1;
    }

    try {
        server->enable_thread_pool(96);
        auto err = server->enable_event_loop(net::EventLoopType::EPOLL);
        if (err.has_value()) {
            std::cerr << "Failed to enable event loop: " << err.value().msg << std::endl;
            return 1;
        }

        auto handler = [server](net::RemoteTarget::SharedPtr remote) {
            std::vector<uint8_t> req;
            auto err = server->read(req, remote);
            if (err.has_value()) {
                std::cerr << std::format("Failed to read from socket {} : {}\n", remote->fd(), err.value().msg);
                return;
            }
            assert(req.size() > 0);
            std::vector<uint8_t> res { req.begin(), req.end() };
            auto err_write = server->write(res, remote);
            if (err_write.has_value()) {
                std::cerr << std::format("Failed to write to socket {} : {}\n", remote->fd(), err.value().msg);
                return;
            }
        };
        server->on_read(handler);
        server->on_start(handler);
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

    net::Timer timer;
    timer.set_rate(10);
    while (true) {
        timer.sleep();
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            std::cout << "Close" << std::endl;
            server->close();
            std::cout << "Close" << std::endl;
            break;
        }
    };

    std::cout << "Server stopped" << std::endl;

    return 0;
}