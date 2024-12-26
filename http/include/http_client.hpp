#pragma once

#include "tcp.hpp"
#include "http_utils.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace http {

class HttpClient {
public:
    HttpClient(const std::string& ip, int port);

    HttpClient(const HttpClient&) = delete;

    HttpClient& operator=(const HttpClient&) = delete;

    HttpClient& operator=(HttpClient&&);

    HttpClient(HttpClient&&);

    ~HttpClient();

    void send_request(const HttpRequest& request);

    HttpResponse recv_response();

    void close();

};



} // namespace http