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

class TcpClient: public Client<std::vector<uint8_t>> {
public:
    TcpClient(const std::string& ip, int port);

    TcpClient(const TcpClient&) = delete;

    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient& operator=(TcpClient&&);

    TcpClient(TcpClient&&);

    ~TcpClient();

    /**
     * @brief Connect to the server
    */
    void connect() override;

    /**
     * @brief Send data to the server
     * 
     * @param data  The data to be sent
    */
    int send(const std::vector<uint8_t>& data) override;

    /**
     * @brief Receive data from the server
     * 
     * @param data  The data to be received
    */
    int recv(std::vector<uint8_t>& data) override;

    /**
     * @brief Close the connection
    */
    void close() override;

private:
    int m_sockfd;
    struct sockaddr_in m_servaddr;

    SocketStatus m_status;
};

class TcpServer: public Server<std::vector<uint8_t>> {
public:
    TcpServer(const std::string& ip, int port);

    TcpServer(const TcpServer&) = delete;

    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer& operator=(TcpServer&&);

    TcpServer(TcpServer&&);

    ~TcpServer();

    void start(std::size_t buffer_size = 1024) override;

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
     * @brief Listen for incoming connections
     * 
     * @param waiting_queue_size  The size of the waiting queue
     */
    void listen(uint32_t waiting_queue_size = 1);
    /**
     * @brief Accept incoming connection
     */
    void accept(struct sockaddr_in* client_addr = nullptr);
    /**
     * @brief Send data to the client
     * 
     * @param data  The data to be sent
     */
    int send(const Connection& conn, const std::vector<uint8_t>& data) override;
    /**
     * @brief Receive data from the client
     * 
     * @param data  The data to be received
     */
    int recv(const Connection& conn, std::vector<uint8_t>& data) override;

    using ClientCallBack = std::function<std::vector<uint8_t>&(const std::vector<uint8_t>&)>;
    std::unordered_map<Connection, ClientCallBack> m_client_callbacks;
};

} // namespace net