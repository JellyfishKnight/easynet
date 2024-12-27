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


    std::thread clientThread([this]() {
        net::HttpClient client("127.0.0.1", 15789);
        try {
            client.connect();
            net::HttpRequest req;
            req.method = net::HttpMethod::GET;
            req.url = "/test";
            // client.send_request(req);
            // auto res = client.recv_response();
            // ASSERT_EQ(static_cast<int>(res.code), 200);
            std::this_thread::sleep_for(std::chrono::seconds(5));
            client.close();
            // server->close();
        } catch (const std::runtime_error& e) {
            FAIL() << e.what();
        }
    });

    server->start();

    

    // clientThread.join();
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}