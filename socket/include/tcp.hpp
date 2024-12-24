#pragma once

#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <string>
#include <vector>

namespace net {

enum class SocketStatus {
    CLOSED,
    CONNECTED,
    LISTENING,
    ACCEPTED
};

class TcpClient {
public:
    TcpClient(const std::string &ip, int port);

    TcpClient(const TcpClient &) = delete;

    TcpClient &operator=(const TcpClient &) = delete;

    TcpClient &operator=(TcpClient &&);

    TcpClient(TcpClient &&);

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
    void send(const std::vector<uint8_t>& data);

    /**
     * @brief Receive data from the server
     * 
     * @param data  The data to be received
    */
    void recv(std::vector<uint8_t>& data);

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
     * @brief Listen for incoming connections
     * 
     * @param waiting_queue_size  The size of the waiting queue
    */
    void listen(uint32_t waiting_queue_size);

    /**
     * @brief Accept incoming connection
    */
    void accept();

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
    void recv(std::vector<uint8_t>& data);

    /**
     * @brief Close the connection
    */
    void close();
private:
    int m_sockfd;

    struct sockaddr_in m_servaddr;
    // struct sockaddr_in m_cliaddr;

    SocketStatus m_status;
};


} // namespace net