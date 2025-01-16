#include "http_server.hpp"
#include "common.hpp"
#include "http_client.hpp"
#include "logger.hpp"
#include "tcp.hpp"
#include "thread_pool.hpp"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace net {

HttpServer::HttpServer(const std::string& ip, int port):
    m_logger_manager(utils::LoggerManager::get_instance()) {
    m_server = std::make_unique<net::TcpServer>(ip, port);
    m_ip = ip;
    m_port = port;
    m_buffer_size = 1024;
    m_logger = m_logger_manager.get_logger("HttpServer");

    m_thread_pool = std::make_unique<utils::ThreadPool>(4);

    m_server->listen(10);
}

HttpServer::HttpServer(HttpServer&& other): m_logger_manager(other.m_logger_manager) {
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
    m_logger = std::move(other.m_logger);
    m_thread_pool = std::move(other.m_thread_pool);

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

void HttpServer::Get(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_get_callbacks[url] = callback;
}

void HttpServer::Post(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_post_callbacks[url] = callback;
}

void HttpServer::Put(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_put_callbacks[url] = callback;
}

void HttpServer::Delete(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_delete_callbacks[url] = callback;
}

void HttpServer::Head(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_head_callbacks[url] = callback;
}

void HttpServer::Options(
    std::string url,
    std::function<HttpResponse(const HttpRequest&)> callback
) {
    m_options_callbacks[url] = callback;
}

void HttpServer::Connect(
    std::string url,
    std::function<HttpResponse(const HttpRequest&)> callback
) {
    m_connect_callbacks[url] = callback;
}

void HttpServer::Trace(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_trace_callbacks[url] = callback;
}

void HttpServer::Patch(std::string url, std::function<HttpResponse(const HttpRequest&)> callback) {
    m_patch_callbacks[url] = callback;
}

void HttpServer::handle_request(const std::string& request_str) {
    auto req = parse_request(request_str);
    if (m_url_callbacks.at(req.method).find(req.url) != m_url_callbacks.at(req.method).end()) {
        if (m_thread_pool_enabled) {
            ///TODO: add the request to the thread pool
        } else {
            send_response(m_url_callbacks.at(req.method).at(req.url)(req));
        }
    } else {
        NET_LOG_ERROR(m_logger, "No callback for the request: {}", req.url);
        return;
    }
}

void HttpServer::start() {
    m_server->accept();
    std::vector<uint8_t> data(m_buffer_size);
    while (m_server->status() != net::SocketStatus::CLOSED) {
        int n;
        try {
            n = m_server->recv(data);
        } catch (const std::runtime_error& e) {
            std::cout << e.what() << std::endl;
            break;
        }
        if (n == -1) {
            NET_LOG_ERROR(
                m_logger,
                "Failed to receive data from the client: {}",
                ::strerror(errno)
            );
            break;
        }
        if (n == 0) {
            NET_LOG_ERROR(m_logger, "Connection reset by peer.");
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
void HttpServer::enable_thread_pool() {
    m_thread_pool_enabled = true;
    m_thread_pool->submit("sendResponseTask", [this]() {
        while (true) {
            if (!m_response_queue.empty()) {
            }
        }
    });
}

void HttpServer::set_buffer_size(std::size_t size) {
    m_buffer_size = size;
}

void HttpServer::disable_thread_pool() {
    m_thread_pool_enabled = false;
}

const net::SocketStatus& HttpServer::status() const {
    return m_server->status();
}

void HttpServer::set_log_path(const std::string& logger_path) {
    m_logger_manager.set_log_path(m_logger, logger_path);
}

} // namespace net