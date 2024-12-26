#pragma once

#include "http_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "tcp.hpp"

namespace net {

class HttpServer {
public:
    HttpServer(const std::string& ip, int port);

    HttpServer(const HttpServer&) = delete;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&);

    HttpServer(HttpServer&&);

    ~HttpServer();

    void handle_request(const std::string &request_str);

    void send_response(const HttpResponse& response);
    
    void add_url_callback(const std::string& url, std::function<void(const HttpRequest&)> callback);

    void close();

    void start(int frequency = 0);
    
    void enable_thread_pool();

    void disable_thread_pool();

    void set_buffer_size(std::size_t size);

    const net::SocketStatus& status() const;

private:
    std::unique_ptr<net::TcpClient> m_server;
    net::SocketStatus m_tcp_status;
    std::string m_ip;
    int m_port;

    std::size_t m_buffer_size;
    bool m_thread_pool_enabled;

    std::unordered_map<std::string, std::function<void(const HttpRequest&)>> m_url_callbacks;

    // thread pool 
    //static 
};



} // namespace net