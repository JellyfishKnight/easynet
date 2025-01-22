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

    HttpResponse
    get(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    std::future<HttpResponse> async_get(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse
    put(const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    std::future<HttpResponse> async_put(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse
    del(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    std::future<HttpResponse> async_del(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& header = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    HttpResponse trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    std::future<HttpResponse> async_trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    void write_req(const HttpRequest& req) final;

    HttpResponse read_res() final;

private:
    std::shared_ptr<HttpParser> m_parser;
};

} // namespace net