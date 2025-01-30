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

/**
 * @brief HttpServer class
 * 
 * This class is used to create a http server
 * @note if you need to handle http failure, you can add error handler, and throw HttpResponseCode in your handler if 
 *       you want to response with error code, and the server will call the error handler you set
 */
class HttpServer {
public:
    HttpServer(std::shared_ptr<TcpServer> server);

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

    int get_fd() const;

    std::string get_ip() const;

    std::string get_service() const;

    ConnectionStatus status() const;

    void add_error_handler(
        HttpResponseCode err_code,
        std::function<HttpResponse(const HttpRequest&)> handler
    );

    ~HttpServer();

    [[nodiscard]] std::shared_ptr<TcpServer> convert2tcp();

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