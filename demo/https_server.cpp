#include "http_server.hpp"
#include "ssl.hpp"
#include <fstream>

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
    net::HttpServer server("127.0.0.1", "8080");

    net::SSLContext::SharedPtr ctx = net::SSLContext::create();
    ctx->set_certificates(
        "/home/jk/Projects/net/keys/certificate.crt",
        "/home/jk/Projects/net/keys/private.key"
    );

    server.add_ssl_context(ctx);

    server.listen();

    auto content = readFileToString("/home/jk/Projects/net/index/index.html");

    server.enable_thread_pool(96);
    server.get("/", [&content](const net::HttpRequest& req) {
        net::HttpResponse res;
        res.version = HTTP_VERSION_1_1;
        res.status_code = net::HttpResponseCode::OK;
        res.reason = "OK";
        res.body = content;
        res.headers["Content-Type"] = "text/html";
        res.headers["Content-Length"] = std::to_string(res.body.size());
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