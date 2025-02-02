#include "http_client.hpp"
#include "enum_parser.hpp"
#include "tcp.hpp"
#include <memory>

int main() {
    net::TcpClient::SharedPtr tcp_client = std::make_shared<net::TcpClient>("127.0.0.1", "8080");
    net::HttpClient client(tcp_client);

    client.connect_server();
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
        auto res = client.get("/" + input);
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