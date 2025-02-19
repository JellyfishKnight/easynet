#include "http_server.hpp"
#include "tcp.hpp"
#include <filesystem>
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

std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        result[count] = '\0';
        return std::string(result);
    }
    return "";
}

int main() {
    net::HttpServer server("127.0.0.1", "8080");

    auto err = server.listen();
    if (err.has_value()) {
        std::cout << "Failed to listening: " + err.value().msg << std::endl;
        return 1;
    }

    auto exe_path = getExecutablePath();
    auto execDir = std::filesystem::path(exe_path).parent_path().string();
    auto content = readFileToString(execDir + "/template/index/index.html");

    server.enable_thread_pool(96);
    server.enable_event_loop();
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