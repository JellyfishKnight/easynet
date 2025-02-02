#include "http_server.hpp"
#include "tcp.hpp"
#include <fstream>
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
    net::HttpServer server(tcp_server);

    server.listen();

    auto content = readFileToString("/home/jk/Projects/net/index/index.html");

    server.enable_thread_pool(96);
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

    server.start();

    while (true) {
        std::string input;
        std::cin >> input;
        // std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            server.close();
            break;
        }
    }
}