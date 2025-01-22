#include "http_client.hpp"
#include "parser.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>

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

HttpResponse HttpClient::get(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.method = HttpMethod::GET;
    req.url = path;
    req.headers = headers;
    req.version = version;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::POST;
    req.headers = headers;
    req.version = version;
    req.url = path;
    req.body = body;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::PUT;
    req.headers = headers;
    req.version = version;
    req.url = path;
    req.body = body;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::DELETE;
    req.headers = headers;
    req.version = version;
    req.url = path;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::PATCH;
    req.headers = headers;
    req.version = version;
    req.url = path;
    req.body = body;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::HEAD;
    req.headers = headers;
    req.version = version;
    req.url = path;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::OPTIONS;
    req.headers = headers;
    req.version = version;
    req.url = path;
    write_req(req);
    return read_res();
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
    req.method = HttpMethod::CONNECT;
    req.headers = headers;
    req.version = version;
    req.url = path;
    write_req(req);
    return read_res();
}

std::future<HttpResponse> HttpClient::async_connect(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    return std::async(std::launch::async, [this, path, &headers, &version]() {
        return connect(path, headers, version);
    });
}

HttpResponse HttpClient::trace(
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& version
) {
    HttpRequest req;
    req.method = HttpMethod::TRACE;
    req.headers = headers;
    req.version = version;
    req.url = path;
    write_req(req);
    return read_res();
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

} // namespace net