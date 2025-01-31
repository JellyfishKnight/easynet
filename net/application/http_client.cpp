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

std::optional<std::string> HttpClient::read_http(HttpResponse& res) {
    std::vector<uint8_t> buffer(1024);
    while (!m_parser->res_read_finished()) {
        buffer.resize(1024);
        auto err = m_client->read(buffer);
        if (err.has_value()) {
            return err;
        }
        m_parser->read_res(buffer, res);
    }
    m_parser->reset_state();
    return std::nullopt;
}

std::optional<std::string> HttpClient::write_http(const HttpRequest& req) {
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

HttpResponse HttpClient::get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::GET).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return get(path, headers, version);
    });
}

HttpResponse HttpClient::post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::POST)
        .set_url(path)
        .set_headers(headers)
        .set_version(version)
        .set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_post(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version]() {
        return post(path, body, headers, version);
    });
}

HttpResponse HttpClient::put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::PUT)
        .set_url(path)
        .set_headers(headers)
        .set_version(version)
        .set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_put(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version]() {
        return put(path, body, headers, version);
    });
}

HttpResponse HttpClient::del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::DELETE).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_del(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return del(path, headers, version);
    });
}

HttpResponse HttpClient::patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::PATCH)
        .set_url(path)
        .set_headers(headers)
        .set_version(version)
        .set_body(body);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_patch(
    const std::string& path,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &body, &headers, &version]() {
        return patch(path, body, headers, version);
    });
}

HttpResponse HttpClient::head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::HEAD).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_head(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return head(path, headers, version);
    });
}

HttpResponse HttpClient::options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::OPTIONS).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_options(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return options(path, headers, version);
    });
}

HttpResponse HttpClient::connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::CONNECT).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return this->connect(path, headers, version);
    });
}

HttpResponse HttpClient::trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.set_method(HttpMethod::TRACE).set_url(path).set_headers(headers).set_version(version);
    auto err = write_http(req);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    HttpResponse res;
    err = read_http(res);
    if (err.has_value()) {
        throw std::runtime_error(err.value());
    }
    return res;
}

std::future<HttpResponse> HttpClient::async_trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return trace(path, headers, version);
    });
}

std::optional<std::string> HttpClient::connect_server() {
    return m_client->connect();
}

std::optional<std::string> HttpClient::close() {
    return m_client->close();
}

HttpClient::~HttpClient() {
    m_parser.reset();
    if (m_client == nullptr) {
        return;
    }
    if (m_client->status() == ConnectionStatus::CONNECTED) {
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

ConnectionStatus HttpClient::status() const {
    return m_client->status();
}

std::shared_ptr<HttpClient> HttpClient::get_shared() {
    return shared_from_this();
}

} // namespace net