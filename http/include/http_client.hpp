#pragma once

#include "http_utils.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tcp.hpp"

namespace net {

class HttpClient {
public:
    HttpClient(const std::string& ip, int port);

    HttpClient(const HttpClient&) = delete;

    HttpClient& operator=(const HttpClient&) = delete;

    HttpClient& operator=(HttpClient&&);

    HttpClient(HttpClient&&);

    ~HttpClient();

    void send_request(const HttpRequest& request);

    HttpResponse recv_response();

    void close();

    void add_response_callback(int id, std::function<void(const HttpResponse& response)> callback);

    const net::SocketStatus& status() const;
    
private:
    std::unique_ptr<net::TcpClient> m_client;
    net::SocketStatus m_tcp_status;
    std::string m_ip;
    int m_port;

    std::unordered_map<int, std::function<void(const HttpResponse& response)>> m_response_callbacks;
};

} // namespace net

