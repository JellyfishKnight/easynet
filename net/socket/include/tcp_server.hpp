#pragma once

#include "connection.hpp"

namespace net {

class TcpServer: public Server<std::vector<uint8_t>, std::vector<uint8_t>, Connection, NoneParser> {
public:
    TcpServer(const std::string& ip, const std::string& service): Server(ip, service) {}

    TcpServer(const TcpServer&) = delete;

    TcpServer(TcpServer&&) = default;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&) = default;

private:
    void write_res(const std::vector<uint8_t>& res, const Connection& fd) final;

    void read_req(std::vector<uint8_t>& req, const Connection& fd) final;

    void handle_connection() final;

    void handle_connection_epoll() final;
};

} // namespace net