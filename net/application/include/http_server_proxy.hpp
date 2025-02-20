#pragma once

#include "defines.hpp"
#include "http_client.hpp"
#include "http_parser.hpp"
#include "http_server.hpp"

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

private:
    virtual void set_handler() override;

    HttpClientGroup::SharedPtr m_clients;
};

} // namespace net