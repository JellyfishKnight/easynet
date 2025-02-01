#include "http_parser.hpp"
#include "tcp.hpp"
#include "websocket.hpp"
#include "websocket_utils.hpp"
#include <thread>

int main() {
    net::TcpClient socket_client("127.0.0.1", "8080");

    net::WebSocketClient client(socket_client.get_shared());

    client.connect_server();

    net::HttpRequest update_req;

    auto res = client.get("/");
    std::cout << "Http version: " << res.version() << std::endl;
    std::cout << "Status Code: " << static_cast<int>(res.status_code()) << std::endl;
    std::cout << "Reason: " << res.reason() << std::endl;
    std::cout << "Headers: " << std::endl;
    for (auto& [key, value]: res.headers()) {
        std::cout << key << ": " << value << std::endl;
    }
    std::cout << "Body: " << res.body() << std::endl;

    update_req.set_version(HTTP_VERSION_1_1)
        .set_method(net::HttpMethod::GET)
        .set_url("/")
        .set_header("Upgrade", "websocket")
        .set_header("Connection", "Upgrade")
        .set_header("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==")
        .set_header("Sec-WebSocket-Version", "13")
        .set_header("Sec-WebSocket-Protocol", "chat");

    auto err = client.upgrade(update_req);
    if (err.has_value()) {
        std::cout << err.value() << std::endl;
        return 1;
    }

    std::thread t([&client]() {
        while (true) {
            net::WebSocketFrame frame;
            auto err = client.read_ws(frame);
            if (err.has_value()) {
                std::cout << err.value() << std::endl;
                return;
            }
            std::cout << "Received: " << frame.payload() << std::endl;
        }
    });

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
        net::WebSocketFrame frame;
        frame.set_opcode(net::WebSocketOpcode::TEXT).set_payload(input);
        client.write_ws(frame);
    }

    return 0;
}