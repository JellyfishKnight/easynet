#include "common.hpp"
#include "http_client.hpp"
#include "tcp.hpp"
#include <iostream>


int main() {
    net::HttpClient client("127.0.0.1", 15238);
    client.connect();
    net::HttpRequest req;
    req.method = net::HttpMethod::GET;
    req.url = "/";
    while (true) {
        std::string s;
        std::cin >> s;
        req.body = s;
        client.send_request(req);
    }
    return 0;
}