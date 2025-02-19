#pragma once

#include "http_parser.hpp"
#include "remote_target.hpp"
#include "socket_base.hpp"
#include "ssl.hpp"
#include "ssl_utils.hpp"
#include "tcp.hpp"
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <tuple>

namespace net {

class HttpClient {
public:
    NET_DECLARE_PTRS(HttpClient)

    HttpClient(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx = nullptr);

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

    void set_proxy(
        const std::string& ip,
        const std::string& service,
        const std::string& username = {},
        const std::string& password = {}
    );

    void unset_proxy();

protected:
    std::shared_ptr<HttpParser> m_parser;
    std::shared_ptr<TcpClient> m_client;

    std::string m_target_ip;
    std::string m_target_service;

    bool m_use_proxy = false;
    std::string m_proxy_ip;
    std::string m_proxy_service;
    std::string m_proxy_username;
    std::string m_proxy_password;

    std::shared_ptr<SSLContext> m_ssl_ctx;
};

class HttpClientGroup {
public:
    HttpClientGroup() = default;

    std::optional<NetError> connect(const std::string& ip, const std::string& service);

    std::optional<NetError> close(const std::string& ip, const std::string& service);

    std::shared_ptr<HttpClient> get_client(const std::string& ip, const std::string& service);

    std::optional<NetError> remove_client(const std::string& ip, const std::string& service);

    std::optional<NetError>
    add_client(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx = nullptr);

    ~HttpClientGroup() = default;

private:
    std::shared_mutex m_mutex;
    std::map<std::tuple<std::string, std::string>, std::shared_ptr<HttpClient>> m_clients;
};

} // namespace net