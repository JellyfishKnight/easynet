#include "tcp.hpp"
#include "websocket.hpp"
#include <memory>

std::string readFileToString(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in);
    if (!file.is_open()) {
        throw std::ios_base::failure("Failed to open file: " + filePath);
    }

    std::ostringstream content;
    content << file.rdbuf(); // 将文件内容全部读取到 stringstream
    file.close();
    return content.str(); // 返回文件内容作为 string
}

int main() {
    net::TcpServer::SharedPtr tcp_server = std::make_shared<net::TcpServer>("127.0.0.1", "8080");

    tcp_server->enable_thread_pool(96);

    net::WebSocketServer server(tcp_server);

    server.allowed_path("/");

    auto content = readFileToString("/home/jk/Projects/net/index/index.html");
    server.get("/", [&content](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.set_version(HTTP_VERSION_1_1)
            .set_status_code(net::HttpResponseCode::OK)
            .set_reason("OK")
            .set_header("Content-Type", "text/html")
            .set_header("Content-Length", std::to_string(content.size()))
            .set_body(content);
        return res;
    });

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