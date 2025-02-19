#include "http_client.hpp"
#include "http_parser.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace net {

HttpClient::HttpClient(std::shared_ptr<TcpClient> client) {
    m_parser = std::make_shared<HttpParser>();
    m_client = std::move(client);
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

std::shared_ptr<HttpClient> HttpClient::get_shared() {
    return shared_from_this();
}

} // namespace net