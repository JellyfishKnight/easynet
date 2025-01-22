#include "http_client.hpp"
#include <string>

namespace net {

HttpClient::HttpClient(const std::string& ip, const std::string& service): Client(ip, service) {
    m_parser = std::make_shared<HttpParser>();
}

HttpResponse HttpClient::read_res() {
    std::vector<uint8_t> buffer(1024);
    HttpResponse res;
    while (!m_parser->res_read_finished()) {
        buffer.resize(1024);
        auto num_bytes = ::recv(m_fd, buffer.data(), buffer.size(), 0);
        if (num_bytes == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to receive data");
        }
        if (num_bytes == 0) {
            throw std::runtime_error("Connection reset by peer while reading");
        }
        buffer.resize(num_bytes);
        m_parser->read_res(buffer, res);
    }
    return res;
}

void HttpClient::write_req(const HttpRequest& req) {
    auto buffer = m_parser->write_req(req);
    auto nums_byte = ::send(m_fd, buffer.data(), buffer.size(), 0);
    if (nums_byte == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to send data");
    }
    if (nums_byte == 0) {
        throw std::runtime_error("Connection reset by peer while writting");
    }
}
} // namespace net