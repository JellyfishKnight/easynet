#pragma once

#include "event_loop.hpp"
#include "http_parser.hpp"
#include "remote_target.hpp"
#include "ssl.hpp"
#include "tcp.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace net {

/**
 * @brief HttpServer class
 * 
 * This class is used to create a http server
 * @note if you need to handle http failure, you can add error handler, and throw HttpResponseCode in your handler if 
 *       you want to response with error code, and the server will call the error handler you set
 */
class HttpServer {
public:
    NET_DECLARE_PTRS(HttpServer)

    HttpServer(const std::string& ip, const std::string& service, std::shared_ptr<SSLContext> ctx = nullptr);

    HttpServer(const HttpServer&) = delete;

    HttpServer(HttpServer&&) = delete;

    HttpServer& operator=(const HttpServer&) = delete;

    HttpServer& operator=(HttpServer&&) = delete;

    virtual ~HttpServer();

    virtual void get(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void post(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void put(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void del(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void head(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void trace(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void connect(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void options(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    virtual void patch(const std::string path, std::function<HttpResponse(const HttpRequest&)> handler);

    std::optional<NetError> listen();

    std::optional<NetError> close();

    std::optional<NetError> start();

    std::optional<NetError> enable_event_loop(EventLoopType type = EventLoopType::EPOLL);

    void enable_thread_pool(std::size_t worker_num);

    void set_logger(const utils::LoggerManager::Logger& logger);

    int get_fd() const;

    std::string get_ip() const;

    std::string get_service() const;

    SocketStatus status() const;

    virtual void add_error_handler(HttpResponseCode err_code, std::function<HttpResponse(const HttpRequest&)> handler);

    [[nodiscard]] std::shared_ptr<TcpServer> convert2tcp();

protected:
    virtual void set_handler();

    virtual void erase_parser(int remote_fd);

    void write_http_res(const HttpResponse& res, RemoteTarget::SharedPtr remote);

    using MethodHandlers = std::unordered_map<std::string, std::function<HttpResponse(const HttpRequest&)>>;
    MethodHandlers m_get_handlers;
    MethodHandlers m_post_handlers;
    MethodHandlers m_put_handlers;
    MethodHandlers m_delete_handlers;
    MethodHandlers m_head_handler;
    MethodHandlers m_trace_handler;
    MethodHandlers m_connect_handler;
    MethodHandlers m_options_handler;
    MethodHandlers m_patch_handler;

    const std::unordered_map<HttpMethod, MethodHandlers&> m_handlers;

    std::map<int, std::shared_ptr<HttpParser>> m_parsers;
    std::mutex m_parsers_mutex;

    std::unordered_map<HttpResponseCode, std::function<HttpResponse(const HttpRequest&)>> m_error_handlers;

    std::shared_ptr<TcpServer> m_server;
};

} // namespace net