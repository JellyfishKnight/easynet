#pragma once

#include "connection.hpp"
#include "parser.hpp"
#include <future>
#include <memory>
#include <string>

namespace net {

class HttpClient: public Client<HttpResponse, HttpRequest> {
public:
    HttpClient(const std::string& ip, const std::string& service);

    HttpClient(const HttpClient&) = delete;

    HttpClient(HttpClient&&) = default;

    HttpClient& operator=(const HttpClient&) = delete;

    HttpClient& operator=(HttpClient&&) = default;

    HttpResponse get(const std::string& path);

    std::future<HttpResponse> async_get(const std::string& path);

    HttpResponse post(const std::string& path, const std::string& body);

    std::future<HttpResponse> async_post(const std::string& path, const std::string& body);

    HttpResponse put(const std::string& path, const std::string& body);

    std::future<HttpResponse> async_put(const std::string& path, const std::string& body);

    HttpResponse del(const std::string& path);

    std::future<HttpResponse> async_del(const std::string& path);

    HttpResponse patch(const std::string& path, const std::string& body);

    std::future<HttpResponse> async_patch(const std::string& path, const std::string& body);

    HttpResponse head(const std::string& path);

    std::future<HttpResponse> async_head(const std::string& path);

    HttpResponse options(const std::string& path);

    std::future<HttpResponse> async_options(const std::string& path);

    HttpResponse connect(const std::string& path);

    std::future<HttpResponse> async_connect(const std::string& path);

    HttpResponse trace(const std::string& path);

    std::future<HttpResponse> async_trace(const std::string& path);

private:
    HttpResponse read_res() final;

    void write_req(const HttpRequest& req) final;

    std::shared_ptr<HttpParser> m_parser;
};

} // namespace net