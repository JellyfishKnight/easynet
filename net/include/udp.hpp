#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "common.hpp"

namespace net {

class UdpClient {
public:
    UdpClient(const std::string& ip, int port);

    UdpClient(const UdpClient&) = delete;

    UdpClient& operator=(const UdpClient&) = delete;

    UdpClient& operator=(UdpClient&&);

    UdpClient(UdpClient&&);

    ~UdpClient();

    /**
     * @brief Get the status object
     * 
     * @return const SocketStatus&
    */
    const SocketStatus& status() const;

    /**
     * @brief Send data to the server
     * 
     * @param data  The data to be sent
    */
    int send(const std::vector<uint8_t>& data);

    /**
     * @brief Receive data from the server
     * 
     * @param data  The data to be received
    */
    int recv(std::vector<uint8_t>& data);

    /**
     * @brief Close the connection
    */
    void close();

private:
    int m_sockfd;
    struct sockaddr_in m_servaddr;
    SocketStatus m_status;
};

class UdpServer {
public:
    UdpServer(const std::string& ip, int port);

    UdpServer(const UdpServer&) = delete;

    UdpServer& operator=(const UdpServer&) = delete;

    UdpServer& operator=(UdpServer&&);

    UdpServer(UdpServer&&);

    ~UdpServer();

    /**
     * @brief Get the status object
     * 
     * @return const SocketStatus&
    */
    const SocketStatus& status() const;

    /**
     * @brief Bind the server to the address
    */
    void bind();

    /**
     * @brief Send data to the client
     * 
     * @param data  The data to be sent
    */
    void send(const std::vector<uint8_t>& data);

    /**
     * @brief Receive data from the client
     * 
     * @param data  The data to be received
    */
    int recv(std::vector<uint8_t>& data);

    /**
     * @brief Close the connection
    */
    void close();

private:
    int m_sockfd;
    struct sockaddr_in m_servaddr;
    SocketStatus m_status;
};

} // namespace net