#include "http_server.hpp"
#include "ssl.hpp"
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
    net::SSLContext::SharedPtr ctx = net::SSLContext::create();
    ctx->set_certificates("/home/jk/Projects/net/keys/certificate.crt", "/home/jk/Projects/net/keys/private.key");

    net::SSLServer::SharedPtr server = std::make_shared<net::SSLServer>(ctx, "127.0.0.1", "8080");

    net::HttpServer http_server(server);

    http_server.listen();

    auto content = readFileToString("/home/jk/Projects/net/index/index.html");

    http_server.enable_thread_pool(96);
    // http_server.enable_epoll(1024);
    http_server.get("/", [&content](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.set_version(HTTP_VERSION_1_1)
            .set_status_code(net::HttpResponseCode::OK)
            .set_reason("OK")
            .set_header("Content-Type", "text/html")
            .set_header("Content-Length", std::to_string(content.size()))
            .set_body(content);
        return res;
    });

    http_server.start();

    while (true) {
        std::string input;
        std::cin >> input;
        // std::cout << "Received input: " << input << std::endl;
        if (input == "exit") {
            http_server.close();
            break;
        }
    }
}