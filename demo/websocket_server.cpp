#include "tcp.hpp"
#include "websocket.hpp"

int main() {
    net::TcpServer tcp_server("127.0.0.1", "8080");

    tcp_server.enable_thread_pool(96);

    net::WebSocketServer server(tcp_server.get_shared());

    server.allowed_path("/");

    server.listen();
    server.start();

    std::string res_str = "this is from server";

    server.add_websocket_handler([&res_str](
                                     net::WebSocketFrame& res,
                                     net::WebSocketFrame& req,
                                     net::Connection::ConstSharedPtr conn
                                 ) {
        res.set_fin(true).set_opcode(net::WebSocketOpcode::TEXT).set_payload(res_str);
    });

    while (true) {
        std::cin >> res_str;
        if (res_str == "exit") {
            server.close();
            return 0;
        }
    }

    return 0;
}