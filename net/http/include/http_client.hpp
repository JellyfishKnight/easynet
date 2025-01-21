#pragma once

#include "connection.hpp"
#include "parser.hpp"

namespace net {

class HttpClient: public Client<HttpResponse, HttpRequest, HttpParser> {
public:
    HttpClient(const std::string& ip, const std::string& service): Client(ip, service) {
        m_parser = std::make_shared<HttpParser>();
    }

    HttpClient(const HttpClient&) = delete;

    HttpClient(HttpClient&&) = default;

    HttpClient& operator=(const HttpClient&) = delete;

    HttpClient& operator=(HttpClient&&) = default;

    HttpResponse read_res() final {
        std::vector<uint8_t> buffer(1024);
        ssize_t num_bytes = ::recv(m_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to receive data");
        }
        if (num_bytes == 0) {
            throw std::runtime_error("Connection reset by peer");
        }
        return m_parser->read_res(buffer);
    }

    void write_req(const HttpRequest& req) final {
        auto buffer = m_parser->write_req(req);
        if (::send(m_fd, buffer.data(), buffer.size(), 0) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
    }
};

class HttpServer: public Server<HttpResponse, HttpRequest, Connection, HttpParser> {
public:
    HttpServer(const std::string& ip, const std::string& service): Server(ip, service) {
        m_parser = std::make_shared<HttpParser>();
    }

    HttpServer(const HttpServer&) = delete;

    HttpServer(HttpServer&&) = default;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&) = default;

private:
    void write_res(const HttpResponse& res, const Connection& fd) final {
        int num_bytes = ::send(fd.m_client_fd, res.data(), res.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
        if (num_bytes == 0) {
            throw std::system_error(
                0,
                std::system_category(),
                "Connection reset by peer while writing"
            );
        }
    }
};

} // namespace net