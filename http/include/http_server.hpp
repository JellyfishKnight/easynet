#pragma once

#include "tcp.hpp"
#include "http_utils.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace http {

template<bool enable_epoll = false>
class HttpServer {
public:
    HttpServer(const std::string& ip, int port);

    HttpServer(const http::HttpServer<enable_epoll>&) = delete;

    HttpServer& operator=(const http::HttpServer<enable_epoll>&) = delete;

    HttpServer& operator=(http::HttpServer<enable_epoll>&&);

    HttpServer(http::HttpServer<enable_epoll>&&);

    ~HttpServer();

    void handle_request(const std::string &request_str);

    void send_response(const HttpResponse& response);
    
    void add_url_callback(const std::string& url, std::function<void(const HttpRequest&)> callback);

    void close();

    void start();

    void enable_thread_pool();

private:
    std::unique_ptr<net::TcpClient> m_server;
    net::SocketStatus m_tcp_status;
    std::string m_ip;
    int m_port;

    std::unordered_map<std::string, std::function<void(const HttpRequest&)>> m_url_callbacks;

    // thread pool 
    //static 
};



} // namespace http