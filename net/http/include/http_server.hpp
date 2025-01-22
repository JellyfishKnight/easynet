#pragma once

#include "connection.hpp"
#include "parser.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace net {

class HttpServer: public Server<HttpResponse, HttpRequest, Connection> {
public:
    HttpServer(const std::string& ip, const std::string& service);

    HttpServer(const HttpServer&) = delete;

    HttpServer(HttpServer&&) = default;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&) = default;

    void get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    void patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

private:
    void write_res(const HttpResponse& res, const Connection& fd) final;

    void read_req(HttpRequest& req, const Connection& fd) final;

    void handle_connection(const Connection& conn) final;

    void handle_connection_epoll() final;

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

    std::shared_ptr<HttpParser> m_parser;
};

} // namespace net