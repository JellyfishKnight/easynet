#include "http_client.hpp"
#include "enum_parser.hpp"
#include "tcp.hpp"
#include <memory>

int main() {
    net::HttpClient client("www.baidu.com", "80");
    client.set_proxy("127.0.0.1", "2196");

    auto err = client.connect_server();
    if (err.has_value()) {
        std::cerr << "Failed to connect to server: " << err.value().msg << std::endl;
        return 1;
    }
    while (true) {
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            client.close();
            return 0;
        }
        if (input == "s") {
            input.clear();
        }
        net::HttpResponse res;
        auto err = client.get(res, "/" + input);
        if (err.has_value()) {
            std::cerr << "Failed to get from server: " << err.value().msg << std::endl;
            continue;
        }

        std::cout << "Http version: " << res.version() << std::endl;
        std::cout << "Status Code: " << static_cast<int>(res.status_code()) << std::endl;
        std::cout << "Reason: " << res.reason() << std::endl;
        std::cout << "Headers: " << std::endl;
        for (auto& [key, value]: res.headers()) {
            std::cout << key << ": " << value << std::endl;
        }
        std::cout << "Body: " << res.body() << std::endl;
    }
}