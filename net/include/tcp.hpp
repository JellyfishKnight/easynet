#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"

namespace net {

class TcpClient {
public:
    TcpClient(const std::string& ip, int port);

    TcpClient(const TcpClient&) = delete;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&);

    TcpClient(TcpClient&&);

    ~TcpClient();

    /**
     * @brief Get the status object
     * 
     * @return const SocketStatus&
    */
    const SocketStatus& status() const;

    /**
     * @brief Change the ip object
     * 
     * @param ip  The new ip address
    */
    void change_ip(const std::string& ip);

    /**
     * @brief Change the port object
     * 
     * @param port  The new port number
    */
    void change_port(int port);

    /**
     * @brief Connect to the server
    */
    void connect();

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

class TcpServer {
public:
    TcpServer(const std::string& ip, int port);

    TcpServer(const TcpServer&) = delete;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&);

    TcpServer(TcpServer&&);

    ~TcpServer();

    /**
     * @brief Get the status object
     * 
     * @return const SocketStatus&
     */
    const SocketStatus& status() const;

    /**
     * @brief Get the address object
     * 
     * @return const struct sockaddr_in&
     */
    const struct sockaddr_in& addr() const;

    /**
     * @brief Listen for incoming connections
     * 
     * @param waiting_queue_size  The size of the waiting queue
     */
    void listen(uint32_t waiting_queue_size = 1);

    void start(std::size_t buffer_size = 1024);

    /**
     * @brief Close the connection
     */
    void close();

    void add_msg_callback(
        const std::string& ip,
        int port,
        std::function<std::vector<uint8_t>&(const std::vector<uint8_t>&)> callback
    );

private:
    /**
     * @brief Accept incoming connection
     */
    void accept(const struct sockaddr_in* const client_addr = nullptr);
    /**
     * @brief Send data to the client
     * 
     * @param data  The data to be sent
     */
    int send(const std::vector<uint8_t>& data);

    /**
     * @brief Receive data from the client
     * 
     * @param data  The data to be received
     */
    int recv(std::vector<uint8_t>& data);

    int m_sockfd;

    struct sockaddr_in m_servaddr;

    SocketStatus m_status;

    std::vector<Connection> m_connections;

    using ClientCallBack = std::function<std::vector<uint8_t>&(const std::vector<uint8_t>&)>;
    std::unordered_map<Connection, ClientCallBack> m_client_connections;

    std::thread m_server_thread;
};

} // namespace net