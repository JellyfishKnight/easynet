#include "http_server_proxy.hpp"
#include "defines.hpp"
#include "http_parser.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"

namespace net {

HttpServerProxyForward::HttpServerProxyForward(
    const std::string& ip,
    const std::string& service,
    std::shared_ptr<SSLContext> ctx
):
    HttpServer(ip, service, ctx) {
    m_clients = std::make_shared<HttpClientGroup>();
    this->set_handler();
}

void HttpServerProxyForward::custom_routed_requests(std::function<void(HttpRequest&)> handler) {
    m_request_custom_handler = handler;
}

bool HttpServerProxyForward::request_url_valid(const HttpRequest& request) {
    // judge if url is valid
    const std::string& url = request.url();
    if (url.empty() || url.find("://") == std::string::npos) {
        return false;
    }
    return true;
}

bool HttpServerProxyForward::https_proxy_required(const HttpRequest& request) {
    return request.url().find("https://") != std::string::npos;
}

std::optional<NetError>
HttpServerProxyForward::get_target_ip_service(const HttpRequest& request, std::string& ip, std::string& service) {
    // find the path in the request
    auto url_start = request.url().find("://");
    if (url_start == std::string::npos) {
        return NetError { NET_HTTP_PROXY_INVALID_URL, "Invalid url" };
    }
    auto ssl_required = https_proxy_required(request);
    auto url_end = request.url().find("/", url_start + 3);
    auto colon_pos = request.url().find(":", url_start + 3);
    if (url_end == std::string::npos) {
        if (colon_pos == std::string::npos) {
            ip = request.url().substr(url_start + 3);
            service = ssl_required ? "443" : "80";
        } else {
            ip = request.url().substr(url_start + 3, colon_pos - url_start - 3);
            service = request.url().substr(colon_pos + 1);
        }
    } else {
        ip = request.url().substr(url_start + 3, url_end - url_start - 3);
        if (colon_pos == std::string::npos || colon_pos > url_end) {
            service = ssl_required ? "443" : "80";
        } else {
            service = request.url().substr(colon_pos + 1, url_end - colon_pos - 1);
        }
    }
    return std::nullopt;
}

void HttpServerProxyForward::set_handler() {
    auto handler_thread_func = [this](RemoteTarget::SharedPtr remote) {
        {
            std::lock_guard<std::mutex> lock_guard(m_parsers_mutex);
            if (!m_parsers.contains(remote->fd())) {
                m_parsers.insert({ remote->fd(), std::make_shared<HttpParser>() });
            }
        }
        auto& parser = m_parsers.at(remote->fd());
        // parse request
        std::vector<uint8_t> req(1024);
        std::vector<uint8_t> res;
        auto err = m_server->read(req, remote);
        if (err.has_value()) {
            std::cerr << std::format("Failed to read from socket: {}\n", err.value().msg) << std::endl;
            erase_parser(remote->fd());
            return;
        }
        std::optional<HttpRequest> req_opt;
        parser->add_req_read_buffer(req);
        while (true) {
            HttpResponse res;
            req_opt = parser->read_req();
            if (!req_opt.has_value()) {
                break;
            }
            auto& request = req_opt.value();
            if (!request_url_valid(request)) {
                // if the url is not valid, return a 400 Bad Request response
                auto handler = m_error_handlers.find(HttpResponseCode::BAD_REQUEST);
                if (handler == m_error_handlers.end()) {
                    res.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::BAD_REQUEST)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                        .set_header("Content-Length", "0");
                } else {
                    res = handler->second(request);
                }
                write_http_res(res, remote);
                continue;
            }

            // find the path in the request
            size_t third_slash_pos = request.url().find("/", request.url().find("/", request.url().find("/") + 1) + 1);
            request.set_url(request.url().substr(third_slash_pos));
            auto target = request.headers().find("host");
            if (target == request.headers().end()) {
                // if the target is not found, return a 400 Bad Request response
                auto handler = m_error_handlers.find(HttpResponseCode::BAD_REQUEST);
                if (handler == m_error_handlers.end()) {
                    res.set_version(HTTP_VERSION_1_1)
                        .set_status_code(HttpResponseCode::BAD_REQUEST)
                        .set_reason(std::string(utils::dump_enum(HttpResponseCode::BAD_REQUEST)))
                        .set_header("Content-Length", "0");
                } else {
                    res = handler->second(request);
                }
                write_http_res(res, remote);
                continue;
            }

            std::string target_ip = target->second.substr(0, target->second.find(':'));
            std::string target_service = target->second.substr(target->second.find(':') + 1);
            auto client = m_clients->get_client(target_ip, target_service);
            if (!client) {
                auto err = m_clients->add_client(target_ip, target_service);
                if (err.has_value()) {
                    std::cerr << std::format("Failed to add client: {}\n", err.value().msg) << std::endl;
                    return;
                }
                client = m_clients->get_client(target_ip, target_service);
                if (!client) {
                    std::cerr << "Failed to get client" << std::endl;
                    return;
                }
            }
            if (client->status() == SocketStatus::DISCONNECTED) {
                err = client->connect_server();
                if (err.has_value()) {
                    std::cerr << std::format("Failed to connect to client: {}\n", err.value().msg) << std::endl;
                    return;
                }
            }
            if (m_request_custom_handler) {
                m_request_custom_handler(request);
            }
            err = client->write_http(request);
            if (err.has_value()) {
                std::cerr << std::format("Failed to write to client: {}\n", err.value().msg) << std::endl;
                return;
            }
            err = client->read_http(res);
            if (err.has_value()) {
                std::cerr << std::format("Failed to read from client: {}\n", err.value().msg) << std::endl;
                return;
            }
            auto buffer = parser->write_res(res);
            err = m_server->write(buffer, remote);
            if (err.has_value()) {
                std::cerr << std::format("Failed to write to socket: {}\n", err.value().msg) << std::endl;
                erase_parser(remote->fd());
                break;
            }
        }
    };

    m_server->on_start(handler_thread_func);
    m_server->on_read(handler_thread_func);
}

} // namespace net