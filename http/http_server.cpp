#include "http_server.hpp"
#include "http_client.hpp"
#include "http_utils.hpp"
#include "tcp.hpp"
#include <chrono>
#include <iostream>
#include <ostream>
#include <thread>

namespace net {

HttpServer::HttpServer(const std::string& ip, int port) {
    m_server = std::make_unique<net::TcpServer>(ip, port);
    m_server->listen(10);
    m_ip = ip;
    m_port = port;
    
}

HttpServer::HttpServer(HttpServer&& other) {
    m_server = std::move(other.m_server);
    m_ip = other.m_ip;
    m_port = other.m_port;
    m_buffer_size = other.m_buffer_size;
    m_thread_pool_enabled = other.m_thread_pool_enabled;
    m_url_callbacks = std::move(other.m_url_callbacks);
}

HttpServer& HttpServer::operator=(HttpServer&& other) {
    if (this != &other) {
        m_server = std::move(other.m_server);
        m_ip = other.m_ip;
        m_port = other.m_port;
        m_buffer_size = other.m_buffer_size;
        m_thread_pool_enabled = other.m_thread_pool_enabled;
        m_url_callbacks = std::move(other.m_url_callbacks);
    }
    return *this;
}

HttpServer::~HttpServer() {
    if (m_server->status() != net::SocketStatus::CLOSED) {
        close();
    }
}

void HttpServer::handle_request(const std::string &request_str) {
    auto req = parse_request(request_str);

    if (m_url_callbacks.find(req.url) != m_url_callbacks.end()) {
        if (m_thread_pool_enabled) {
            ///TODO: add the request to the thread pool
        } else {
            m_url_callbacks[req.url](req);        
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

void HttpServer::start(int frequency) {
    m_loop_thread = std::thread([this, frequency] {
        m_server->accept(); 
        if (frequency == 0) {
            while (true) {
                if (m_server->status() == net::SocketStatus::CLOSED) {
                    break;
                }
                std::vector<uint8_t> data(m_buffer_size);
                m_server->recv(data);
                std::string request_str(data.begin(), data.end());
                handle_request(request_str);
            }
        } else {
            while (true) {
                if (m_server->status() == net::SocketStatus::CLOSED) {
                    break;
                }
                std::vector<uint8_t> data(m_buffer_size);
                auto n = m_server->recv(data);
                if (n == -1) {
                    // log error
                    continue;
                }
                std::string request_str(data.begin(), data.end());
                handle_request(request_str);
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1.0 / frequency * 1000)));
            }
        }
    });
}

void HttpServer::send_response(const HttpResponse& response) {
    std::string res = create_response(response);
    std::vector<uint8_t> data(res.begin(), res.end());
    m_server->send(data);
}

void HttpServer::add_url_callback(const std::string& url, std::function<void(const HttpRequest&)> callback) {
    m_url_callbacks[url] = callback;
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