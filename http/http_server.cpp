#include "http_server.hpp"
#include "http_client.hpp"
#include "http_utils.hpp"

namespace http {

template<bool enbale_epoll>
HttpServer<enbale_epoll>::HttpServer(const std::string& ip, int port) {
    m_server = std::make_unique<net::TcpClient>(ip, port);
    m_server->connect();
    m_tcp_status = m_server->status();
    m_ip = ip;
    m_port = port;
}

template<bool enbale_epoll>
HttpServer<enbale_epoll>::~HttpServer() {
    if (m_tcp_status != net::SocketStatus::CLOSED) {
        close();
    }
}

template<bool enbale_epoll>
void HttpServer<enbale_epoll>::handle_request(const std::string &request_str) {
    auto req = parse_request(request_str);

    if (m_url_callbacks.find(req.url) != m_url_callbacks.end()) {
        m_url_callbacks[req.url](req);
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


} // namespace http