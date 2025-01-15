#include <cstdio>
#include <gtest/gtest.h>
#include "tcp.hpp"
#include <thread>
#include <chrono>
#include <iostream>

// 测试 TcpServer 类
class TcpServerTest : public ::testing::Test {
protected:
    // 创建一个 TcpServer 实例，并在其上启动监听
    void SetUp() override {
        try {
            server = std::make_unique<net::TcpServer>("127.0.0.1", 15238);
            server->listen(10);
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    }

    // 关闭服务器
    void TearDown() override {
        try {
            server->close();
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    }

    std::unique_ptr<net::TcpServer> server;
};

// 测试连接功能
TEST_F(TcpServerTest, TestServerAcceptConnection) {
    // 服务器需要先监听
    try {
        ASSERT_EQ(server->status(), net::SocketStatus::LISTENING);
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    // 创建客户端连接
    net::TcpClient client("127.0.0.1", 15238);
    try {
        client.connect();
        ASSERT_EQ(client.status(), net::SocketStatus::CONNECTED);
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    // 接受客户端连接
    try {
        server->accept();
        ASSERT_EQ(server->status(), net::SocketStatus::CONNECTED);
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    client.close();
}

// 测试发送和接收数据
TEST_F(TcpServerTest, TestSendRecvData) {
    // 创建客户端并连接到服务器
    std::thread clientThread([]() {
        net::TcpClient client("127.0.0.1", 15238);
        client.connect();

        std::vector<uint8_t> sendData = { 1, 2, 3, 4, 5 };

        
        client.send(sendData);

        std::vector<uint8_t> recvData(5);
        client.recv(recvData);


        ASSERT_EQ(recvData, sendData);  // 检查发送和接收的数据是否一致

        client.close();
    });

    // 启动服务器并接受连接
    server->accept();

    // 接收客户端发送的数据
    std::vector<uint8_t> recvData(8);
    server->recv(recvData);

    // 向客户端发送响应数据
    server->send(recvData);

    // 等待客户端线程完成
    clientThread.join();
}

// 测试客户端断开连接
TEST_F(TcpServerTest, TestServerClose) {
    // 创建客户端并连接到服务器
    std::thread clientThread([]() {
        net::TcpClient client("127.0.0.1", 15238);
        client.connect();
        ASSERT_EQ(client.status(), net::SocketStatus::CONNECTED);
        client.close();
    });

    // 启动服务器并接受连接
    server->accept();
    ASSERT_EQ(server->status(), net::SocketStatus::CONNECTED);

    // 关闭服务器
    server->close();
    ASSERT_EQ(server->status(), net::SocketStatus::CLOSED);

    // 等待客户端线程完成
    clientThread.join();
}

// 测试客户端的 IP 和端口变更
TEST_F(TcpServerTest, TestChangeIPandPort) {
    net::TcpClient client("127.0.0.1", 15238);
    client.connect();
    ASSERT_EQ(client.status(), net::SocketStatus::CONNECTED);


    client.close();
    // 改变客户端的 IP 和端口
    client.change_ip("127.0.0.1");
    client.change_port(9090);

    server->close();
    ASSERT_EQ(server->status(), net::SocketStatus::CLOSED);
    server->change_ip("127.0.0.1");
    server->change_port(9090);
    server->listen(10);

    // 重新连接
    client.connect();
    ASSERT_EQ(client.status(), net::SocketStatus::CONNECTED);

    client.close();
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}