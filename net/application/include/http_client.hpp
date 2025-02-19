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

    virtual std::optional<NetError>
    get(HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<std::optional<NetError>> async_get(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> post(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_post(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError>
    put(HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<std::optional<NetError>> async_put(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError>
    del(HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1);

    virtual std::future<std::optional<NetError>> async_del(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> patch(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_patch(
        HttpResponse& response,
        const std::string& path,
        const std::string& body,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> head(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_head(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> options(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_options(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> connect(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& header = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_connect(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> trace(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::future<std::optional<NetError>> async_trace(
        HttpResponse& response,
        const std::string& path,
        const std::unordered_map<std::string, std::string>& headers = {},
        const std::string& version = HTTP_VERSION_1_1
    );

    virtual std::optional<NetError> write_http(const HttpRequest& req);

    virtual std::optional<NetError> read_http(HttpResponse& res);

    std::optional<NetError> connect_server();

    std::optional<NetError> close();

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