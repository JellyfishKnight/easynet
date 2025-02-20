#pragma once

#include "defines.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"
#include <functional>
#include <tuple>

namespace net {

class HttpServerProxyForward: public HttpServer {
public:
    NET_DECLARE_PTRS(HttpServerProxyForward)

    HttpServerProxyForward(
        const std::string& ip,
        const std::string& service,
        std::shared_ptr<SSLContext> ctx = nullptr
    );

    virtual ~HttpServerProxyForward() = default;

    /**
     * @brief this function can add custom request routed to the target server
     * @param headers the headers to add
     */
    void custom_routed_requests(std::function<void(HttpRequest&)> handler);

private:
    bool request_url_valid(const HttpRequest& request);

    std::optional<NetError> get_target_ip_service(const HttpRequest& request, std::string& ip, std::string& service);

    bool https_proxy_required(const HttpRequest& request);

    void set_handler() override;

    std::function<void(HttpRequest&)> m_request_custom_handler = nullptr;

    HttpClientGroup::SharedPtr m_clients;
};

} // namespace net