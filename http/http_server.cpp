#include "http_server.hpp"
#include "http_client.hpp"
#include "http_utils.hpp"
#include "tcp.hpp"
#include <chrono>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <thread>

namespace net {

HttpServer::HttpServer(const std::string& ip, int port) {
    m_server = std::make_unique<net::TcpServer>(ip, port);
    m_ip = ip;
    m_port = port;
    m_buffer_size = 1024;

    m_server->listen(10);
}

HttpServer::HttpServer(HttpServer&& other) {
    m_server = std::move(other.m_server);
    m_ip = other.m_ip;
    m_port = other.m_port;
    m_buffer_size = other.m_buffer_size;
    m_thread_pool_enabled = other.m_thread_pool_enabled;

    m_get_callbacks = std::move(other.m_get_callbacks);
    m_post_callbacks = std::move(other.m_post_callbacks);
    m_put_callbacks = std::move(other.m_put_callbacks);
    m_delete_callbacks = std::move(other.m_delete_callbacks);
    m_head_callbacks = std::move(other.m_head_callbacks);
    m_options_callbacks = std::move(other.m_options_callbacks);
    m_connect_callbacks = std::move(other.m_connect_callbacks);
    m_trace_callbacks = std::move(other.m_trace_callbacks);
    m_patch_callbacks = std::move(other.m_patch_callbacks);

    other.m_get_callbacks.clear();
    other.m_post_callbacks.clear();
    other.m_put_callbacks.clear();
    other.m_delete_callbacks.clear();
    other.m_head_callbacks.clear();
    other.m_options_callbacks.clear();
    other.m_connect_callbacks.clear();
    other.m_trace_callbacks.clear();
    other.m_patch_callbacks.clear();
}

HttpServer& HttpServer::operator=(HttpServer&& other) {
    if (this != &other) {
        m_server = std::move(other.m_server);
        m_ip = other.m_ip;
        m_port = other.m_port;
        m_buffer_size = other.m_buffer_size;
        m_thread_pool_enabled = other.m_thread_pool_enabled;

        m_get_callbacks = std::move(other.m_get_callbacks);
        m_post_callbacks = std::move(other.m_post_callbacks);
        m_put_callbacks = std::move(other.m_put_callbacks);
        m_delete_callbacks = std::move(other.m_delete_callbacks);
        m_head_callbacks = std::move(other.m_head_callbacks);
        m_options_callbacks = std::move(other.m_options_callbacks);
        m_connect_callbacks = std::move(other.m_connect_callbacks);
        m_trace_callbacks = std::move(other.m_trace_callbacks);
        m_patch_callbacks = std::move(other.m_patch_callbacks);

        other.m_get_callbacks.clear();
        other.m_post_callbacks.clear();
        other.m_put_callbacks.clear();
        other.m_delete_callbacks.clear();
        other.m_head_callbacks.clear();
        other.m_options_callbacks.clear();
        other.m_connect_callbacks.clear();
        other.m_trace_callbacks.clear();
        other.m_patch_callbacks.clear();
    }
    return *this;
}

HttpServer::~HttpServer() {
    if (m_server->status() != net::SocketStatus::CLOSED) {
        close();
    }
}

void HttpServer::Get(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_get_callbacks[url] = callback;
}

void HttpServer::Post(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_post_callbacks[url] = callback;
}

void HttpServer::Put(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_put_callbacks[url] = callback;
}

void HttpServer::Delete(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_delete_callbacks[url] = callback;
}

void HttpServer::Head(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_head_callbacks[url] = callback;
}

void HttpServer::Options(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_options_callbacks[url] = callback;
}

void HttpServer::Connect(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_connect_callbacks[url] = callback;
}

void HttpServer::Trace(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_trace_callbacks[url] = callback;
}

void HttpServer::Patch(std::string url, std::function<void(const HttpRequest&)> callback) {
    m_patch_callbacks[url] = callback;
}

void HttpServer::handle_request(const std::string &request_str) {
    auto req = parse_request(request_str);
    if (m_url_callbacks.at(req.method).find(req.url) != m_url_callbacks.at(req.method).end()) {
        if (m_thread_pool_enabled) {
            ///TODO: add the request to the thread pool
        } else {
            m_url_callbacks.at(req.method).at(req.url)(req);        
        }
    } else {
        // return 404 not found
        HttpResponse response;
        response.code = HttpReturnCode::NOT_FOUND;
        response.body = "404 Not Found";
        response.version = "HTTP/1.1";
        response.headers["Content-Length"] = std::to_string(response.body.size());
        response.headers["Content-Type"] = "text/plain";
        send_response(response);
    }
}

void HttpServer::start() {
    m_server->accept();
    std::vector<uint8_t> data(m_buffer_size);
    while (m_server->status() != net::SocketStatus::CLOSED) {
        int n ;
        try {
            n = m_server->recv(data);
        } catch (const std::runtime_error& e) {
            std::cout << e.what() << std::endl;
            break;
        }
        if (n == -1) {
            // log error
            continue;
        }
        if (n == 0) {
            // log error
            std::cout << "Connection reset by peer." << std::endl;
            break;
        }
        std::string request_str(data.begin(), data.end());
        handle_request(request_str);
    }
}

void HttpServer::send_response(const HttpResponse& response) {
    std::string res = create_response(response);
    std::vector<uint8_t> data(res.begin(), res.end());
    m_server->send(data);
}

void HttpServer::close() {
    m_server->close();
}

/*not complete*/
void HttpServer::enable_thread_pool() {}

void HttpServer::set_buffer_size(std::size_t size) {
    m_buffer_size = size;
}

void HttpServer::disable_thread_pool() {}

const net::SocketStatus& HttpServer::status() const {
    return m_server->status();
}

} // namespace net