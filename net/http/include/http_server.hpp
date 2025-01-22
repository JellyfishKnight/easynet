#pragma once

#include "connection.hpp"

namespace net {

class HttpServer: public Server<HttpResponse, HttpRequest, Connection> {
public:
    HttpServer(const std::string& ip, const std::string& service): Server(ip, service) {
        m_parser = std::make_shared<HttpParser>();
    }

    HttpServer(const HttpServer&) = delete;

    HttpServer(HttpServer&&) = default;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&) = default;

private:
    void write_res(const HttpResponse& res, const Connection& fd) final;

    void read_req(HttpRequest& req, const Connection& fd) final;

    void handle_connection(const Connection& conn) final;

    void handle_connection_epoll() final;

    std::shared_ptr<HttpParser> m_parser;
};

} // namespace net