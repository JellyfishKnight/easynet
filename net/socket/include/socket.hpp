#pragma once

#include "connection.hpp"
#include "parser.hpp"
#include <cstdint>
#include <future>
#include <sys/types.h>
#include <vector>

namespace net {

class TcpClient: public Client<std::vector<uint8_t>, std::vector<uint8_t>> {
public:
    TcpClient(const std::string& ip, const std::string& service): Client(ip, service) {}

    TcpClient(const TcpClient&) = delete;

    TcpClient(TcpClient&&) = default;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&) = default;

    std::vector<uint8_t> read_res() final;

    void write_req(const std::vector<uint8_t>& req) final;
};

class TcpServer: public Server<std::vector<uint8_t>, std::vector<uint8_t>, Connection> {
public:
    TcpServer(const std::string& ip, const std::string& service): Server(ip, service) {}

    TcpServer(const TcpServer&) = delete;

    TcpServer(TcpServer&&) = default;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&) = default;

private:
    void write_res(const std::vector<uint8_t>& res, const Connection& fd) final;

    void read_req(std::vector<uint8_t>& req, const Connection& fd) final;

    void handle_connection(const Connection& conn) final;

    void handle_connection_epoll(const struct ::epoll_event& event) final;
};

} // namespace net