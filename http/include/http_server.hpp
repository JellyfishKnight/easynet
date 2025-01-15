#pragma once

#include "http_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "logger.hpp"
#include "tcp.hpp"
#include "thread_pool.hpp"

namespace net {

class HttpServer {
public:
    HttpServer(const std::string& ip, int port);

    HttpServer(const HttpServer&) = delete;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&);

    HttpServer(HttpServer&&);

    ~HttpServer();

    void send_response(const HttpResponse& response);

    void Get(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Post(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Put(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Delete(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Head(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Options(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Connect(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Trace(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    void Patch(std::string url, std::function<HttpResponse(const HttpRequest&)> callback);

    // void listen();

    void close();

    void start();

    void enable_thread_pool();

    void disable_thread_pool();

    void set_buffer_size(std::size_t size);

    const net::SocketStatus& status() const;

    void set_log_path(const std::string& logger_path);

private:
    void handle_request(const std::string& request_str);

    std::unique_ptr<net::TcpServer> m_server;
    std::string m_ip;
    int m_port;

    std::size_t m_buffer_size;
    bool m_thread_pool_enabled;

    // call back group
    using CallBackWithRoute =
        std::unordered_map<std::string, std::function<void(const HttpRequest&)>>;
    CallBackWithRoute m_get_callbacks;
    CallBackWithRoute m_post_callbacks;
    CallBackWithRoute m_put_callbacks;
    CallBackWithRoute m_delete_callbacks;
    CallBackWithRoute m_head_callbacks;
    CallBackWithRoute m_options_callbacks;
    CallBackWithRoute m_connect_callbacks;
    CallBackWithRoute m_trace_callbacks;
    CallBackWithRoute m_patch_callbacks;

    const std::unordered_map<HttpMethod, CallBackWithRoute&> m_url_callbacks = {
        { HttpMethod::GET, m_get_callbacks },         { HttpMethod::POST, m_post_callbacks },
        { HttpMethod::PUT, m_put_callbacks },         { HttpMethod::DELETE, m_delete_callbacks },
        { HttpMethod::HEAD, m_head_callbacks },       { HttpMethod::OPTIONS, m_options_callbacks },
        { HttpMethod::CONNECT, m_connect_callbacks }, { HttpMethod::TRACE, m_trace_callbacks },
        { HttpMethod::PATCH, m_patch_callbacks },
    };

    std::thread m_loop_thread;

    // logger
    utils::LoggerManager::Logger m_logger;
    utils::LoggerManager& m_logger_manager;

    // thread pool
};

} // namespace net