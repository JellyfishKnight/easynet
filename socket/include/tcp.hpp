#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <string>

namespace net {

class TcpClient {
public:
    TcpClient(const std::string &ip, int port);

    TcpClient(const TcpClient &) = delete;

    TcpClient &operator=(const TcpClient &) = delete;

    TcpClient &operator=(TcpClient &&);

    TcpClient(TcpClient &&);

    ~TcpClient();

    void connect(const char* ip, int port);
    void send(const char* data, int size);
    void recv(char* data, int size);
    void close();

private:
    int m_sockfd;
    struct sockaddr_in m_servaddr;
};

class TcpServer {
public:
    TcpServer();
    ~TcpServer();

    void bind(int port);
    void listen();
    void accept();
    void send(const char* data, int size);
    void recv(char* data, int size);
    void close();
private:
    int m_sockfd;
    struct sockaddr_in m_servaddr;
    struct sockaddr_in m_cliaddr;
};


} // namespace net