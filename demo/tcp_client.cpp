#include "enum_parser.hpp"
#include "parser.hpp"
#include "print.hpp"
#include "socket.hpp"
#include <ostream>
#include <string>

std::string generate_large_post_request() {
    std::string method = "POST";
    std::string url = "/submit-data";
    std::string version = "HTTP/1.1";
    std::string host = "www.example.com";
    std::string user_agent = "CustomUserAgent/1.0";
    std::string content_type = "application/x-www-form-urlencoded";

    // 构造 body 内容，填充大量数据
    std::string body = "key1=value1&key2=value2&data=";
    for (int i = 0; i < 1500; ++i) {
        body += "A"; // 填充 'A' 来模拟大数据
    }

    // 计算 Content-Length
    std::string content_length = std::to_string(body.size());

    // 构造 HTTP 请求头和 body
    std::string request = method + " " + url + " " + version + "\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: " + user_agent + "\r\n";
    request += "Content-Type: " + content_type + "\r\n";
    request += "Content-Length: " + content_length + "\r\n";
    request += "Connection: close\r\n\r\n";
    request += body;

    return request;
}

int main() {
    std::string request = generate_large_post_request();
    net::http_request_parser http_parser;

    auto start = request.begin();
    auto buffer_length = 1024;
    std::cout << request.size() << std::endl;
    try {
        while (start < request.end()) {
            if (request.end() - start < 1024) {
                buffer_length = request.end() - start;
            }
            auto end = start + buffer_length;
            if (end > request.end()) {
                end = request.end();
            }
            std::string buffer { start, end };
            http_parser.push_chunk(buffer);
            if (!http_parser.request_finished()) {
                start = end;
            } else {
                break;
            }
        }
    } catch (const std::out_of_range& e) {
        std::cerr << "Failed to parse request: " << e.what() << std::endl;
        return 1;
    }

    net::HttpRequest req;
    req.version = http_parser.version();
    req.url = http_parser.url();
    req.headers = http_parser.headers();
    req.is_complete = true;
    req.method = http_parser.method();
    req.body = std::move(http_parser.body());

    std::cout << "Request method: " << utils::dump_enum(req.method) << std::endl;

    std::cout << "Request URL: " << req.url << std::endl;
    std::cout << "Request version: " << req.version << std::endl;
    for (auto it = req.headers.begin(); it != req.headers.end(); ++it) {
        std::cout << "Request header: " << it->first << ": " << it->second << std::endl;
    }
    std::cout << "Request body: " << req.body << std::endl;

    // net::TcpClient client("127.0.0.1", "8080");

    // client.connect();

    // while (true) {
    //     std::string input;
    //     std::cin >> input;
    //     std::vector<uint8_t> req { input.begin(), input.end() };
    //     client.write_req(req);
    //     auto res = client.read_res();
    //     std::string res_str { res.begin(), res.end() };
    //     std::cout << res_str << std::endl;
    //     if (input == "exit") {
    //         break;
    //     }
    // }

    return 0;
}