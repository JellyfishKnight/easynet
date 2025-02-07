#pragma once

#include "http_parser.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace net {

class HttpClient: public std::enable_shared_from_this<HttpClient> {
public:
    NET_DECLARE_PTRS(HttpClient)

    HttpClient(std::shared_ptr<TcpClient> client);

    HttpClient(const HttpClient&) = delete;

    HttpClient(HttpClient&&) = default;

    HttpClient& operator=(const HttpClient&) = delete;

    HttpClient& operator=(HttpClient&&) = default;

    virtual ~HttpClient();

    virtual HttpResponse
    get(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<HttpResponse> async_get(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_post(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse
    put(const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<HttpResponse> async_put(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse
    del(const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<HttpResponse> async_del(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_patch(
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_head(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_options(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& header = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_connect(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual HttpResponse trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<HttpResponse> async_trace(
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<std::string> write_http(const HttpRequest& req);

    virtual std::optional<std::string> read_http(HttpResponse& res);

    std::optional<std::string> connect_server();

    std::optional<std::string> close();

    int get_fd() const;

    SocketType type() const;

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    std::shared_ptr<HttpClient> get_shared();

protected:
    std::shared_ptr<HttpParser> m_parser;
    std::shared_ptr<TcpClient> m_client;
};

} // namespace net