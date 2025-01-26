#include "http_client.hpp"
#include "enum_parser.hpp"
#include <memory>

int main() {
    net::HttpClient client("127.0.0.1", "8080");

    client.connect_server();
    while (true) {
        std::string input;
        std::cin >> input;
        if (input == "exit") {
            client.close();
            return 0;
        }
        auto res = client.get("/" + input);
        std::cout << "Http version: " << res.version << std::endl;
        std::cout << "Status Code: " << static_cast<int>(res.status_code) << std::endl;
        std::cout << "Reason: " << res.reason << std::endl;
        std::cout << "Headers: " << std::endl;
        for (auto& [key, value]: res.headers) {
            std::cout << key << ": " << value << std::endl;
        }
        std::cout << "Body: " << res.body << std::endl;
    }
}