#include "remote_target.hpp"
#include "tcp.hpp"
#include "timer.hpp"
#include "websocket.hpp"
#include <filesystem>
#include <memory>
#include <sys/types.h>
#include <thread>

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
    auto exe_path = getExecutablePath();
    auto execDir = std::filesystem::path(exe_path).parent_path().string();

    net::TcpServer::SharedPtr tcp_server = std::make_shared<net::TcpServer>();

    net::WebSocketServer server("127.0.0.1", "8080");

    server.allowed_path("/");
    server.enable_thread_pool(96);

    auto content = readFileToString(execDir + "/template/index/index.html");
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

    net::Timer timer;
    timer.set_rate(10);

    server.add_websocket_handler([&server, &res_str, &timer](net::RemoteTarget::SharedPtr remote) {
        static uint64_t count = 0;
        res_str = "this is from server " + std::to_string(count++);
        timer.sleep();
        net::WebSocketFrame frame;
        frame.set_fin(1)
            .set_rsv1(0)
            .set_rsv2(0)
            .set_rsv3(0)
            .set_opcode(net::WebSocketOpcode::TEXT)
            .set_payload(res_str);

        auto err = server.write_websocket_frame(frame, remote);
        if (err.has_value()) {
            std::cerr << "Failed to write to socket: " << err.value().msg << std::endl;
            return;
        }

        std::cout << "Sent: " + res_str << std::endl;
    });

    while (true) {
        std::cin >> res_str;
        std::cout << "input: " + res_str << std::endl;
        if (res_str == "exit") {
            server.close();
            return 0;
        }
    }

    return 0;
}