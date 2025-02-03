#include "http_client.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <memory>

int main() {
    auto ctx = net::SSLContext::create();
    ctx->set_certificates(
        "/home/jk/Projects/net/keys/certificate.crt",
        "/home/jk/Projects/net/keys/private.key"
    );

    net::SSLClient::SharedPtr ssl_client =
        std::make_shared<net::SSLClient>(ctx, "127.0.0.1", "8080");

    net::HttpClient client(ssl_client);

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

    return 0;
}