#include "http_client.hpp"
#include "defines.hpp"
#include "http_parser.hpp"
#include "print.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include "websocket_utils.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace net {

HttpClient::HttpClient(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx):
    m_parser(std::make_shared<HttpParser>()),
    m_target_ip(ip),
    m_target_service(service),
    m_ssl_ctx(ctx) {
    if (ctx) {
        m_client = std::make_shared<SSLClient>(ctx, m_target_ip, m_target_service);
    } else {
        m_client = std::make_shared<TcpClient>(m_target_ip, m_target_service);
    }
}

std::optional<NetError> HttpClient::read_http(HttpResponse& res) {
    std::vector<uint8_t> buffer(1024);
    std::optional<HttpResponse> res_opt;
    do {
        buffer.resize(1024);
        auto err = m_client->read(buffer);
        if (err.has_value()) {
            return err;
        }
        m_parser->add_res_read_buffer(buffer);
        res_opt = m_parser->read_res();
    } while (!res_opt.has_value());
    res = std::move(res_opt.value());
    return std::nullopt;
}

std::optional<NetError> HttpClient::write_http(const HttpRequest& req) {
    if (m_use_proxy) {
        // Prepare the proxy request (for example, set the proxy URL and headers)
        auto& proxy_req = const_cast<HttpRequest&>(req);
        if (!m_proxy_username.empty() && !m_proxy_password.empty()) {
            proxy_req.set_headers({ { "Proxy-Authorization",
                                      "Basic " + base64_encode(m_proxy_username + ":" + m_proxy_password) } });
        }
        proxy_req.set_headers({ { "Host", m_target_ip + ":" + m_target_service } });
        proxy_req.set_url("http://" + m_target_ip + ":" + m_target_service + req.url());
    }
    auto buffer = m_parser->write_req(req);
    if (buffer.empty()) {
        return std::nullopt;
    }
    auto err = m_client->write(buffer);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::optional<NetError> HttpClient::get(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::GET).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_get(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return get(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::post(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::POST).set_url(path).set_headers(headers).set_version(version).set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_post(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version, &response]() {
        return post(response, path, body, headers, version);
    });
}

std::optional<NetError> HttpClient::put(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::PUT).set_url(path).set_headers(headers).set_version(version).set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_put(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version, &response]() {
        return put(response, path, body, headers, version);
    });
}

std::optional<NetError> HttpClient::del(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::DELETE).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_del(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return del(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::patch(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::PATCH).set_url(path).set_headers(headers).set_version(version).set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_patch(
    HttpResponse& response,
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version, &response]() {
        return patch(response, path, body, headers, version);
    });
}

std::optional<NetError> HttpClient::head(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::HEAD).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_head(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return head(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::options(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::OPTIONS).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_options(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return options(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::connect(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::CONNECT).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_connect(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return this->connect(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::trace(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::TRACE).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        return err;
    }
    err = read_http(response);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::future<std::optional<NetError>> HttpClient::async_trace(
    HttpResponse& response,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version, &response]() {
        return trace(response, path, headers, version);
    });
}

std::optional<NetError> HttpClient::connect_server() {
    return m_client->connect();
}

std::optional<NetError> HttpClient::close() {
    return m_client->close();
}

HttpClient::~HttpClient() {
    m_parser.reset();
    if (m_client == nullptr) {
        return;
    }
    if (m_client->status() == SocketStatus::CONNECTED) {
        m_client->close();
    }
    m_client.reset();
}

int HttpClient::get_fd() const {
    return m_client->get_fd();
}

SocketType HttpClient::type() const {
    return m_client->type();
}

std::string HttpClient::get_ip() const {
    return m_client->get_ip();
}

std::string HttpClient::get_service() const {
    return m_client->get_service();
}

SocketStatus HttpClient::status() const {
    return m_client->status();
}

void HttpClient::set_proxy(
    const std::string& ip,
    const std::string& service,
    const std::string& username,
    const std::string& password
) {
    m_use_proxy = true;
    m_proxy_ip = ip;
    m_proxy_service = service;
    m_proxy_username = username;
    m_proxy_password = password;
    if (m_ssl_ctx) {
        m_client = std::make_shared<SSLClient>(m_ssl_ctx, m_proxy_ip, m_proxy_service);
    } else {
        m_client = std::make_shared<TcpClient>(m_proxy_ip, m_proxy_service);
    }
}

void HttpClient::unset_proxy() {
    m_use_proxy = false;
    m_proxy_ip.clear();
    m_proxy_service.clear();
    m_proxy_username.clear();
    m_proxy_password.clear();
    if (m_ssl_ctx) {
        m_client = std::make_shared<SSLClient>(m_ssl_ctx, m_target_ip, m_target_service);
    } else {
        m_client = std::make_shared<TcpClient>(m_target_ip, m_target_service);
    }
}

std::optional<NetError> HttpClientGroup::connect(const std::string& ip, const std::string& service) {
    auto client = get_client(ip, service);
    if (!client) {
        return NetError { NET_NO_CLIENT_FOUND, "No client found int group" };
    }
    auto err = add_client(ip, service);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::optional<NetError> HttpClientGroup::close(const std::string& ip, const std::string& service) {
    auto client = get_client(ip, service);
    if (!client) {
        return NetError { NET_NO_CLIENT_FOUND, "No client found int group" };
    }
    auto err = remove_client(ip, service);
    if (err.has_value()) {
        return err;
    }
    return std::nullopt;
}

std::shared_ptr<HttpClient> HttpClientGroup::get_client(const std::string& ip, const std::string& service) {
    std::shared_lock lock(m_mutex);
    auto it = m_clients.find(std::make_tuple(ip, service));
    if (it == m_clients.end()) {
        return nullptr;
    }
    return it->second;
}

std::optional<NetError> HttpClientGroup::remove_client(const std::string& ip, const std::string& service) {
    std::unique_lock lock(m_mutex);
    auto it = m_clients.find(std::make_tuple(ip, service));
    if (it == m_clients.end()) {
        return NetError { NET_NO_CLIENT_FOUND, "No client found in group" };
    }
    m_clients.erase(it);
    return std::nullopt;
}

std::optional<NetError>
HttpClientGroup::add_client(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx) {
    std::unique_lock lock(m_mutex);
    auto it = m_clients.find(std::make_tuple(ip, service));
    if (it != m_clients.end()) {
        return NetError { NET_CLIENT_ALREADY_EXISTS, "Client already exists in group" };
    }
    auto client = std::make_shared<HttpClient>(ip, service, ctx);
    m_clients[std::make_tuple(ip, service)] = client;
    return std::nullopt;
}

} // namespace net