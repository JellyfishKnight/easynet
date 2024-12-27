#include "http_client.hpp"
#include <stdexcept>
#include <iostream>
namespace net {

HttpClient::HttpClient(const std::string& ip, int port) {
    m_client = std::make_unique<net::TcpClient>(ip, port);
    m_ip = ip;
    m_port = port;
}

HttpClient::HttpClient(HttpClient&& other) {
    m_client = std::move(other.m_client);
    m_ip = other.m_ip;
    m_port = other.m_port;
}

HttpClient& HttpClient::operator=(HttpClient&& other) {
    if (this != &other) {
        m_client = std::move(other.m_client);
        m_ip = other.m_ip;
        m_port = other.m_port;
    }
    return *this;
}

HttpClient::~HttpClient() {
    if (m_client->status() != net::SocketStatus::CLOSED) {
        close();
    }
}

void HttpClient::send_request(const HttpRequest& request) {
    std::string req_str = create_request(request);
    std::vector<uint8_t> data(req_str.begin(), req_str.end());
    std::cout << m_client->send(data);
}

HttpResponse HttpClient::recv_response() {
    std::vector<uint8_t> data(1024);
    m_client->recv(data);
    std::string res_str(data.begin(), data.end());
    return parse_response(res_str);
}

void HttpClient::add_response_callback(int id, std::function<void(const HttpResponse& response)> callback) {
    m_response_callbacks[id] = callback;
}

const net::SocketStatus& HttpClient::status() const {
    return m_client->status();
}

void HttpClient::close() {
    m_client->close();
}

void HttpClient::connect() {
    m_client->connect();
}

} // namespace net