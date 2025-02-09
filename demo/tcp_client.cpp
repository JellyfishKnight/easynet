#include "enum_parser.hpp"
#include "print.hpp"
#include "socket.hpp"
#include "thread_pool.hpp"
#include <cstdint>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

int main() {
    net::TcpClient client("127.0.0.1", "8080");

    auto err = client.connect();
    if (err.has_value()) {
        std::cerr << "Failed to connect to server: " << err.value() << std::endl;
        return 1;
    }

    while (true) {
        std::string input;
        std::cin >> input;
        std::vector<uint8_t> req { input.begin(), input.end() };
        client.write(req);
        std::vector<uint8_t> res(1024);
        client.read(res);
        std::string res_str { res.begin(), res.end() };
        std::cout << res_str << std::endl;
        if (input == "exit") {
            break;
        }
    }

    return 0;
}