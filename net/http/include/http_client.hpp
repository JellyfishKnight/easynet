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
            throw std::runtime_error("Connection reset by peer while reading");
        }
        return m_parser->read_res(buffer);
    }

    void write_req(const HttpRequest& req) final {
        auto buffer = m_parser->write_req(req);
        auto nums_byte = ::send(m_fd, buffer.data(), buffer.size(), 0);
        if (nums_byte == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
        if (nums_byte == 0) {
            throw std::runtime_error("Connection reset by peer while writting");
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
        auto res_buffer = m_parser->write_res(res);
        int num_bytes = ::send(fd.m_client_fd, res_buffer.data(), res_buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to send data");
        }
        if (num_bytes == 0) {
            throw std::runtime_error("Connection reset by peer while writing");
        }
    }

    void read_req(HttpRequest& req, const Connection& fd) final {
        while (!req.is_complete) {
            std::vector<uint8_t> buffer(1024);
            ssize_t num_bytes = ::recv(fd.m_client_fd, buffer.data(), buffer.size(), 0);
            if (num_bytes == -1) {
                throw std::system_error(errno, std::system_category(), "Failed to receive data");
            }
            if (num_bytes == 0) {
                throw std::runtime_error("Connection reset by peer while reading");
            }
            buffer.resize(num_bytes);
            req = m_parser->read_req(buffer);
        }
    }
};

} // namespace net