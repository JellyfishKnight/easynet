#include "enum_parser.hpp"
#include "print.hpp"
#include "socket.hpp"
#include "thread_pool.hpp"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

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
    auto ctx = net::SSLContext::create();
    auto exe_path = getExecutablePath();
    auto execDir = std::filesystem::path(exe_path).parent_path().string();

    ctx->set_certificates(execDir + "/template/keys/certificate.crt", execDir + "/template/keys/private.key");

    net::SSLClient::SharedPtr client = std::make_shared<net::SSLClient>(ctx, "127.0.0.1", "8080");

    auto err = client->connect();
    if (err.has_value()) {
        std::cerr << "Failed to connect to server: " << err.value() << std::endl;
        return 1;
    }

    while (true) {
        std::string input;
        std::cin >> input;
        std::vector<uint8_t> req { input.begin(), input.end() };
        client->write(req);
        std::vector<uint8_t> res;
        client->read(res);
        if (res.empty()) {
            continue;
        }
        std::string res_str { res.begin(), res.end() };
        std::cout << res_str << std::endl;
        if (input == "exit") {
            break;
        }
    }

    return 0;
}