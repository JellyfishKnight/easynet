#include "socket.hpp"

int main() {
    net::TcpClient client("127.0.0.1", "8080");

    client.connect();

    while (true) {
        std::string input;
        std::cin >> input;
        std::vector<uint8_t> req { input.begin(), input.end() };
        client.write_req(req);
        auto res = client.read_res();
        std::string res_str { res.begin(), res.end() };
        std::cout << res_str << std::endl;
        if (input == "exit") {
            break;
        }
    }

    return 0;
}