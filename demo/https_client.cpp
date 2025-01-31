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

    net::SSLClient ssl_client(ctx, "www.baidu.com", "443");

    net::HttpClient client(ssl_client.get_shared());

    // client.add_ssl_context(ctx);

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