#include "enum_parser.hpp"
#include "print.hpp"
#include "socket.hpp"
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

int main() {
    net::TcpClient client("127.0.0.1", "8080");

    client.connect();

    while (true) {
        std::string input;
        std::cin >> input;
        std::vector<uint8_t> req { input.begin(), input.end() };
        client.write(req);
        std::vector<uint8_t> res;
        client.read(res);
        std::string res_str { res.begin(), res.end() };
        std::cout << res_str << std::endl;
        if (input == "exit") {
            break;
        }
    }

    return 0;
}