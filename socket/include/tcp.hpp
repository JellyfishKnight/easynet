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

    const SocketStatus& status() const;

    void change_ip(const std::string& ip);

    void change_port(int port);

    void connect();

    void send(const std::vector<uint8_t>& data);

    void recv(std::vector<uint8_t>& data);
    
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

    const SocketStatus& status() const;

    void change_ip(const std::string& ip);

    void change_port(int port);

    void listen(uint32_t waiting_queue_size);

    void accept();

    void send(const std::vector<uint8_t>& data);

    void recv(std::vector<uint8_t>& data);
    
    void close();
private:
    int m_sockfd;

    struct sockaddr_in m_servaddr;
    struct sockaddr_in m_cliaddr;

    SocketStatus m_status;
};


} // namespace net