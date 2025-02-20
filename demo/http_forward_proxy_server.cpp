#include "http_server_proxy.hpp"
#include <memory>

int main() {
    net::HttpServerProxyForward::SharedPtr server = std::make_shared<net::HttpServerProxyForward>("127.0.0.1", "2196");

    server->enable_event_loop();
    server->enable_thread_pool(96);
    server->add_error_handler(net::HttpResponseCode::BAD_REQUEST, [](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.set_version(HTTP_VERSION_1_1)
            .set_status_code(net::HttpResponseCode::BAD_REQUEST)
            .set_reason("Bad Request")
            .set_header("Content-Length", "0");
        return res;
    });

    server->add_error_handler(net::HttpResponseCode::NOT_FOUND, [](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.set_version(HTTP_VERSION_1_1)
            .set_status_code(net::HttpResponseCode::NOT_FOUND)
            .set_reason("Not Found")
            .set_header("Content-Length", "0");
        return res;
    });

    auto err = server->listen();
    if (err.has_value()) {
        std::cerr << "Failed to listen: " << err.value().msg << std::endl;
        return 1;
    }

    err = server->start();
    if (err.has_value()) {
        std::cerr << "Failed to start: " << err.value().msg << std::endl;
        return 1;
    }

    while (true) {
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            server->close();
            return 0;
        }
        if (input == "s") {
            input.clear();
        }
    }
    return 0;
}