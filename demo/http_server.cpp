#include "http_server.hpp"
#include "parser.hpp"

int main() {
    net::HttpServer server("127.0.0.1", "8080");

    server.listen();

    server.enable_thread_pool(10);
    server.get("/", [](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.version = HTTP_VERSION_1_1;
        res.status_code = net::HttpResponseCode::OK;
        res.reason = "OK";
        res.headers["Content-Type"] = "text/plain";
        res.body = "Hello, World!";
        return res;
    });

    server.start();

    while (true) {
        std::string input;
        std::cin >> input;
        // std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            server.stop();
            break;
        }
    }
}