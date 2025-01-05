#include "http_client.hpp"
#include "http_server.hpp"
#include "http_utils.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>

class HttpTest: public ::testing::Test {
protected:
    void SetUp() override {
        try {
            server = std::make_unique<net::HttpServer>("127.0.0.1", 15789);
        } catch (const std::runtime_error& e) {
            FAIL() << e.what();
        }
    }

    void TearDown() override {
        try {
            server->close();
        } catch (const std::runtime_error& e) {
            FAIL() << e.what();
        }
    }

    std::unique_ptr<net::HttpServer> server;
};


TEST_F(HttpTest, TestServerAcceptConnection) {
    try {
        ASSERT_EQ(server->status(), net::SocketStatus::LISTENING);
    } catch (const std::runtime_error& e) {
        FAIL() << e.what();
    }


    std::thread clientThread([]() {
        net::HttpClient client("127.0.0.1", 15789);
        try {
            client.connect();
            net::HttpRequest req;
            req.method = net::HttpMethod::GET;
            req.url = "/test";
            req.version = "HTTP/1.1";
            req.body = "Hello, World!";
            req.headers["Content-Length"] = std::to_string(req.body.size());
            client.send_request(req);
            auto res = client.recv_response();
            ASSERT_EQ(static_cast<int>(res.code), 404);
            client.close();
        } catch (const std::runtime_error& e) {
            FAIL() << e.what();
        }
    });

    std::thread serverThread([this]() {
        try {
            server->start();
        } catch (const std::runtime_error& e) {
            FAIL() << e.what();
        }
    });

    clientThread.join();
    serverThread.join();
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}