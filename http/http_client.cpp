#include "http_client.hpp"

namespace http {

HttpClient::HttpClient(const std::string& ip, int port) {
    m_client = std::make_unique<net::TcpClient>(ip, port);
    m_client->connect();
    m_tcp_status = m_client->status();
    m_ip = ip;
    m_port = port;
}

HttpClient::HttpClient(HttpClient&& other) {
    m_client = std::move(other.m_client);
    m_tcp_status = other.m_tcp_status;
    m_ip = other.m_ip;
    m_port = other.m_port;
}

HttpClient& HttpClient::operator=(HttpClient&& other) {
    if (this != &other) {
        m_client = std::move(other.m_client);
        m_tcp_status = other.m_tcp_status;
        m_ip = other.m_ip;
        m_port = other.m_port;
    }
    return *this;
}

HttpClient::~HttpClient() {
    if (m_tcp_status != net::SocketStatus::CLOSED) {
        close();
    }
}

void HttpClient::send_request(const HttpRequest& request) {
    std::string req_str = create_request(request);
    std::vector<uint8_t> data(req_str.begin(), req_str.end());
    m_client->send(data);
}

HttpResponse HttpClient::recv_response() {
    std::vector<uint8_t> data(1024);
    auto n = m_client->recv(data);
    if (n == -1) {
        throw std::runtime_error("Failed to receive data");
    }
    std::string res_str(data.begin(), data.end());
    return parse_response(res_str);
}

void HttpClient::add_response_callback(int id, std::function<void(const HttpResponse& response)> callback) {
    m_response_callbacks[id] = callback;
}


} // namespace http