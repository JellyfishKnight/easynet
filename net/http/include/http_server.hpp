#pragma once

#include "connection.hpp"
#include "http_parser.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace net {

class HttpServer {
public:
    HttpServer(const std::string& ip, const std::string& service);

    HttpServer(const HttpServer&) = delete;

    HttpServer(HttpServer&&) = default;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&) = delete;

    void get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    std::optional<std::string> listen();

    std::optional<std::string> close();

    std::optional<std::string> start();

    void enable_thread_pool(std::size_t worker_num);

    std::optional<std::string> enable_epoll(std::size_t event_num);

    void set_logger(const utils::LoggerManager::Logger& logger);

    int get_fd() const;

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

    /**
     * @brief Add an SSL context to the server
     * @note This function should be called before calling connect_server, and once called this function, 
    *        the settings of server will be reset 
     *       (like thread pool, epoll, etc.)
     */
    void add_ssl_context(std::shared_ptr<SSLContext> ctx);

    void add_error_handler(
        HttpResponseCode err_code,
        std::function<HttpResponse(const HttpRequest&)> handler
    );

private:
    void set_handler();

    using MethodHandlers =
        std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>;
    MethodHandlers m_get_handlers;
    MethodHandlers m_post_handlers;
    MethodHandlers m_put_handlers;
    MethodHandlers m_delete_handlers;
    MethodHandlers m_head_handler;
    MethodHandlers m_trace_handler;
    MethodHandlers m_connect_handler;
    MethodHandlers m_options_handler;
    MethodHandlers m_patch_handler;

    const std::unordered_map<HttpMethod, MethodHandlers&> m_handlers;

    std::unordered_map<ConnectionKey, std::shared_ptr<HttpParser>> m_parsers;

    std::unordered_map<HttpResponseCode, std::function<HttpResponse(const HttpRequest&)>>
        m_error_handlers;

    std::shared_ptr<TcpServer> m_server;
};

} // namespace net