#include "http_client.hpp"
#include "http_server.hpp"
#include "http_utils.hpp"

#include <gtest/gtest.h>
#include <thread>

class HttpTest: public ::testing::Test {
protected:
    void SetUp() override {
        try {
            server = std::make_unique<net::HttpServer>("127.0.0.1", 15238);
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    }

    void TearDown() override {
        try {
            server->close();
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    }

    std::unique_ptr<net::HttpServer> server;
};


TEST_F(HttpTest, TestServerAcceptConnection) {
    try {
        ASSERT_EQ(server->status(), net::SocketStatus::LISTENING);
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    server->start();

    std::thread clientThread([]() {
        net::HttpClient client("127.0.0.1", 15238);
        client.connect();
        try {
            net::HttpRequest req;
            req.method = net::HttpMethod::GET;
            req.url = "/test";
            client.send_request(req);
            auto res = client.recv_response();
            ASSERT_EQ(static_cast<int>(res.code), 200);
        } catch (const std::exception& e) {
            FAIL() << e.what();
        }
    });


    clientThread.join();
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}