#pragma once

#include "connection.hpp"
#include "http_parser.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <cstdint>
#include <functional>
#include <memory>
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

    // Set server settings by get this server
    TcpServer& server();

    void add_ssl_context(std::shared_ptr<SSLContext> ctx);

private:
    void handler(std::vector<uint8_t>& res, const std::vector<uint8_t>& req);

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

    std::shared_ptr<TcpServer> m_server;
};

} // namespace net