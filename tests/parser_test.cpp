#include "enum_parser.hpp"
#include "http_parser.hpp"
#include "websocket_utils.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

class ParserTest: public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

public:
    net::HttpParser http_parser;
    net::WebSocketParser websocket_parser;
};

TEST_F(ParserTest, HttpRequestWriteTest) {
    net::HttpRequest req;
    std::string body = "this is a post test";
    req.set_method(net::HttpMethod::POST)
        .set_url("/test")
        .set_version(HTTP_VERSION_1_1)
        .set_header("Content-Length", std::to_string(body.size()))
        .set_body(body);

    auto req_buffer = http_parser.write_req(req);

    std::string wanted_buffer =
        "POST /test HTTP/1.1\r\nContent-Length: 19\r\n\r\nthis is a post test";

    ASSERT_EQ(std::string(req_buffer.begin(), req_buffer.end()), wanted_buffer);
}

TEST_F(ParserTest, HttpResponseWriteTest) {
    net::HttpResponse res;
    std::string body = "this is a post test";
    res.set_status_code(net::HttpResponseCode::OK)
        .set_reason(std::string(utils::dump_enum(net::HttpResponseCode::OK)))
        .set_version(HTTP_VERSION_1_1)
        .set_header("Content-Length", std::to_string(body.size()))
        .set_body(body);

    auto res_buffer = http_parser.write_res(res);

    std::string wanted_buffer = "HTTP/1.1 200 OK\r\nContent-Length: 19\r\n\r\nthis is a post test";

    ASSERT_EQ(std::string(res_buffer.begin(), res_buffer.end()), wanted_buffer);
}

TEST_F(ParserTest, HttpRequestReadTest) {
    std::string buffer = "POST /test HTTP/1.1\r\nContent-Length: 19\r\n\r\nthis is a post test";
    net::HttpRequest req;
    std::vector<uint8_t> buffer_data(buffer.begin(), buffer.end());
    http_parser.read_req(buffer_data, req);
    while (!http_parser.req_read_finished()) {
    };
    ASSERT_EQ(req.method(), net::HttpMethod::POST);
    ASSERT_EQ(req.url(), "/test");
    ASSERT_EQ(req.version(), HTTP_VERSION_1_1);
    ASSERT_EQ(req.header("content-length"), "19");
    ASSERT_EQ(req.body(), "this is a post test");
}

TEST_F(ParserTest, HttpResponseReadTest) {
    std::string buffer = "HTTP/1.1 200 OK\r\nContent-Length: 19\r\n\r\nthis is a post test";

    net::HttpResponse res;
    std::vector<uint8_t> buffer_data(buffer.begin(), buffer.end());

    http_parser.read_res(buffer_data, res);
    while (!http_parser.res_read_finished()) {
    };
    ASSERT_EQ(res.status_code(), net::HttpResponseCode::OK);
    ASSERT_EQ(res.reason(), std::string(utils::dump_enum(net::HttpResponseCode::OK)));
    ASSERT_EQ(res.version(), HTTP_VERSION_1_1);
    ASSERT_EQ(res.header("content-length"), "19");
    ASSERT_EQ(res.body(), "this is a post test");
}

TEST_F(ParserTest, HttpRequestLongReadTest) {
    std::string body;
    for (int i = 0; i < 1000; i++) {
        body += "this is a buffer test";
    }

    std::string buffer =
        "POST /test HTTP/1.1\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    net::HttpRequest req;
    std::vector<uint8_t> buffer_data(buffer.begin(), buffer.end());
    http_parser.read_req(buffer_data, req);
    while (!http_parser.req_read_finished()) {
    };

    ASSERT_EQ(req.method(), net::HttpMethod::POST);
    ASSERT_EQ(req.url(), "/test");
    ASSERT_EQ(req.version(), HTTP_VERSION_1_1);
    ASSERT_EQ(req.header("content-length"), std::to_string(body.size()));
    ASSERT_EQ(req.body(), body);
}

TEST_F(ParserTest, HttpResponseLongReadTest) {
    std::string body;
    for (int i = 0; i < 1000; i++) {
        body += "this is a buffer test";
    }

    std::string buffer =
        "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    net::HttpResponse res;
    std::vector<uint8_t> buffer_data(buffer.begin(), buffer.end());
    http_parser.read_res(buffer_data, res);
    while (!http_parser.res_read_finished()) {
    };

    ASSERT_EQ(res.status_code(), net::HttpResponseCode::OK);
    ASSERT_EQ(res.reason(), std::string(utils::dump_enum(net::HttpResponseCode::OK)));
    ASSERT_EQ(res.version(), HTTP_VERSION_1_1);
    ASSERT_EQ(res.header("content-length"), std::to_string(body.size()));
    ASSERT_EQ(res.body(), body);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}