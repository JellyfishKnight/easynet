#include <gtest/gtest.h>
#include <thread>
#include "udp.hpp"


// 测试 UdpServer 类
class UdpServerTest: public ::testing::Test {
protected:
    // 创建一个 UdpServer 实例，并在其上启动监听
    void SetUp() override {
        try {
            server = std::make_unique<net::UdpServer>("127.0.0.1", 15238);
            server->bind();
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

    std::unique_ptr<net::UdpServer> server;
};

TEST_F(UdpServerTest, TestServerAcceptConnection) {
    // 服务器需要先监听
    try {
        ASSERT_EQ(server->status(), net::SocketStatus::CONNECTED);
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    std::thread serverThread([this]() {
            // 接受客户端连接
        try {
            std::vector<uint8_t> data(5);
            server->recv(data);
            ASSERT_EQ(data, std::vector<uint8_t>({ 1, 2, 3, 4, 5 }));
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    });
                             

    // 创建客户端连接
    std::thread clientThread([]() {
        net::UdpClient client("127.0.0.1", 15238);
        try {
            client.send({ 1, 2, 3, 4, 5 });
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    });

    clientThread.join();
    serverThread.join();
}

TEST_F(UdpServerTest, TestChangeIPandPort) {
    net::UdpClient client("127.0.0.1", 15238);
    try {
        client.send({1, 2, 3, 4, 5});
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    std::vector<uint8_t> data(5);
    server->recv(data);
    ASSERT_EQ(data, std::vector<uint8_t>({ 1, 2, 3, 4, 5 }));

    client.close();
    // 改变客户端的 IP 和端口
    client.change_ip("127.0.0.1");
    client.change_port(15238);
    client.send({ 2, 3, 4, 5, 6 });

    server->recv(data);
    ASSERT_EQ(data, std::vector<uint8_t>({ 2, 3, 4, 5, 6 }));
    client.close();
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}