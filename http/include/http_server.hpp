#pragma once

#include "tcp.hpp"
#include "http_utils.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace http {

class HttpServer {
public:
    HttpServer(const std::string& ip, int port);

    HttpServer(const http::HttpServer&) = delete;

    HttpServer& operator=(const http::HttpServer&) = delete;

    HttpServer& operator=(http::HttpServer&&);

    HttpServer(http::HttpServer&&);

    ~HttpServer();


private:
    std::unique_ptr<net::TcpClient> m_server;
    net::SocketStatus m_tcp_status;
    std::string m_ip;
    int m_port;

    
};



} // namespace http