#pragma once

#include "logger.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace net {

enum class SocketStatus : int { CLOSED = 0, CONNECTED, LISTENING, ACCEPTED };

enum class ConnectionType : uint8_t { TCP = 0, UDP };

struct Connection {
    int client_sock;
    int server_sock;

    std::string client_ip;
    int client_port;

    ConnectionType type;
    SocketStatus status = SocketStatus::CLOSED;

    Connection() = default;

    Connection(int client_sock, int server_sock, ConnectionType c_type):
        client_sock(client_sock),
        server_sock(server_sock),
        type(c_type) {}

    Connection(const std::string& ip, int port, ConnectionType c_type):
        client_ip(ip),
        client_port(port),
        type(c_type) {}

    bool operator==(const Connection& other) const {
        return (client_sock == other.client_sock && server_sock == other.server_sock)
            || (client_ip == other.client_ip && client_port == other.client_port);
    }
};

template<typename DataType = std::vector<uint8_t>>
class Server {
public:
    Server();

    virtual void start(std::size_t buffer_size = 1024) = 0;

    virtual ~Server();

protected:
    virtual int send(const Connection& conn, const DataType& data) = 0;

    virtual int recv(const Connection& conn, DataType& data) = 0;

    virtual std::vector<uint8_t> handle_data_type(const DataType& data) = 0;

    std::vector<Connection> m_client_connections;
    std::thread m_server_thread;
    int m_sockfd;
    struct sockaddr_in m_servaddr;

    SocketStatus m_status = SocketStatus::CLOSED;

    // logger
    utils::LoggerManager::Logger m_logger;
};

template<typename DataType = std::vector<uint8_t>>
class Client {
public:
    Client();

    virtual ~Client();

    virtual void connect();

    virtual void close();

    virtual int send(const DataType& data) = 0;

    virtual int recv(DataType& data) = 0;

    const SocketStatus& status() const;

protected:
    int m_sockfd;
    struct sockaddr_in m_servaddr;
    SocketStatus m_status;

    utils::LoggerManager::Logger m_logger;
};

enum class HttpReturnCode : uint16_t {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,
    EARLY_HINTS = 103,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,
    ALREADY_REPORTED = 208,
    IM_USED = 226,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    IM_A_TEAPOT = 418,
    MISDIRECTED_REQUEST = 421,
    UNPROCESSABLE_ENTITY = 422,
    LOCKED = 423,
    FAILED_DEPENDENCY = 424,
    TOO_EARLY = 425,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 428,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    UNAVAILABLE_FOR_LEGAL_REASONS = 451,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 507,
    LOOP_DETECTED = 508,
};

inline constexpr auto get_return_code_string = [](const HttpReturnCode& code) -> std::string {
    const static std::unordered_map<HttpReturnCode, std::string> return_code_string = {
        { HttpReturnCode::CONTINUE, "Continue" },
        { HttpReturnCode::SWITCHING_PROTOCOLS, "Switching Protocols" },
        { HttpReturnCode::PROCESSING, "Processing" },
        { HttpReturnCode::EARLY_HINTS, "Early Hints" },
        { HttpReturnCode::OK, "OK" },
        { HttpReturnCode::CREATED, "Created" },
        { HttpReturnCode::ACCEPTED, "Accepted" },
        { HttpReturnCode::NON_AUTHORITATIVE_INFORMATION, "Non-Authoritative Information" },
        { HttpReturnCode::NO_CONTENT, "No Content" },
        { HttpReturnCode::RESET_CONTENT, "Reset Content" },
        { HttpReturnCode::PARTIAL_CONTENT, "Partial Content" },
        { HttpReturnCode::MULTI_STATUS, "Multi-Status" },
        { HttpReturnCode::ALREADY_REPORTED, "Already Reported" },
        { HttpReturnCode::IM_USED, "IM Used" },
        { HttpReturnCode::MULTIPLE_CHOICES, "Multiple Choices" },
        { HttpReturnCode::MOVED_PERMANENTLY, "Moved Permanently" },
        { HttpReturnCode::FOUND, "Found" },
        { HttpReturnCode::SEE_OTHER, "See Other" },
        { HttpReturnCode::NOT_MODIFIED, "Not Modified" },
        { HttpReturnCode::USE_PROXY, "Use Proxy" },
        { HttpReturnCode::SWITCH_PROXY, "Switch Proxy" },
        { HttpReturnCode::TEMPORARY_REDIRECT, "Temporary Redirect" },
        { HttpReturnCode::PERMANENT_REDIRECT, "Permanent Redirect" },
        { HttpReturnCode::BAD_REQUEST, "Bad Request" },
        { HttpReturnCode::UNAUTHORIZED, "Unauthorized" },
        { HttpReturnCode::PAYMENT_REQUIRED, "Payment Required" },
        { HttpReturnCode::FORBIDDEN, "Forbidden" },
        { HttpReturnCode::NOT_FOUND, "Not Found" },
        { HttpReturnCode::METHOD_NOT_ALLOWED, "Method Not Allowed" },
        { HttpReturnCode::NOT_ACCEPTABLE, "Not Acceptable" },
        { HttpReturnCode::PROXY_AUTHENTICATION_REQUIRED, "Proxy Authentication Required" },
        { HttpReturnCode::REQUEST_TIMEOUT, "Request Timeout" },
        { HttpReturnCode::CONFLICT, "Conflict" },
        { HttpReturnCode::GONE, "Gone" },
        { HttpReturnCode::LENGTH_REQUIRED, "Length" },
        { HttpReturnCode::PRECONDITION_FAILED, "Precondition Failed" },
        { HttpReturnCode::PAYLOAD_TOO_LARGE, "Payload Too Large" },
        { HttpReturnCode::URI_TOO_LONG, "URI Too Long" },
        { HttpReturnCode::UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type" },
        { HttpReturnCode::RANGE_NOT_SATISFIABLE, "Range Not Satisfiable" },
        { HttpReturnCode::EXPECTATION_FAILED, "Expectation Failed" },
        { HttpReturnCode::IM_A_TEAPOT, "I'm a teapot" },
        { HttpReturnCode::MISDIRECTED_REQUEST, "Misdirected Request" },
        { HttpReturnCode::UNPROCESSABLE_ENTITY, "Unprocessable Entity" },
        { HttpReturnCode::LOCKED, "Locked" },
        { HttpReturnCode::FAILED_DEPENDENCY, "Failed Dependency" },
        { HttpReturnCode::TOO_EARLY, "Too Early" },
        { HttpReturnCode::UPGRADE_REQUIRED, "Upgrade Required" },
        { HttpReturnCode::PRECONDITION_REQUIRED, "Precondition Required" },
        { HttpReturnCode::TOO_MANY_REQUESTS, "Too Many Requests" },
        { HttpReturnCode::REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large" },
        { HttpReturnCode::UNAVAILABLE_FOR_LEGAL_REASONS, "Unavailable For Legal Reasons" },
        { HttpReturnCode::INTERNAL_SERVER_ERROR, "Internal Server Error" },
        { HttpReturnCode::NOT_IMPLEMENTED, "Not Implemented" },
        { HttpReturnCode::BAD_GATEWAY, "Bad Gateway" },
        { HttpReturnCode::SERVICE_UNAVAILABLE, "Service Unavailable" },
        { HttpReturnCode::GATEWAY_TIMEOUT, "Gateway Timeout" },
        { HttpReturnCode::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported" },
        { HttpReturnCode::VARIANT_ALSO_NEGOTIATES, "Variant Also Negotiates" },
        { HttpReturnCode::INSUFFICIENT_STORAGE, "Insufficient Storage" },
        { HttpReturnCode::LOOP_DETECTED, "Loop Detected" },
    };
    return return_code_string.at(code);
};

struct HttpResponse {
    HttpReturnCode code;
    std::string body;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
};

std::string create_response(const HttpResponse& response);

HttpResponse parse_response(const std::string& response);

enum class HttpMethod : uint8_t {
    GET = 0,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    CONNECT,
    TRACE,
    PATCH
};

inline constexpr auto get_string_from_http_method = [](const HttpMethod& method) -> std::string {
    const static std::unordered_map<HttpMethod, std::string> method_map = {
        { HttpMethod::GET, "GET" },         { HttpMethod::POST, "POST" },
        { HttpMethod::PUT, "PUT" },         { HttpMethod::DELETE, "DELETE" },
        { HttpMethod::HEAD, "HEAD" },       { HttpMethod::OPTIONS, "OPTIONS" },
        { HttpMethod::CONNECT, "CONNECT" }, { HttpMethod::TRACE, "TRACE" },
        { HttpMethod::PATCH, "PATCH" },
    };
    return method_map.at(method);
};

inline constexpr auto get_http_method_from_string = [](const std::string& method) -> HttpMethod {
    const static std::unordered_map<std::string, HttpMethod> method_map = {
        { "GET", HttpMethod::GET },         { "POST", HttpMethod::POST },
        { "PUT", HttpMethod::PUT },         { "DELETE", HttpMethod::DELETE },
        { "HEAD", HttpMethod::HEAD },       { "OPTIONS", HttpMethod::OPTIONS },
        { "CONNECT", HttpMethod::CONNECT }, { "TRACE", HttpMethod::TRACE },
        { "PATCH", HttpMethod::PATCH },
    };
    return method_map.at(method);
};

enum class HttpVersion : uint8_t { HTTP_1_0 = 0, HTTP_1_1, HTTP_2_0 };

struct HttpRequest {
    std::string url;
    HttpMethod method;
    std::string body;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
};

std::string create_request(const HttpRequest& request);

HttpRequest parse_request(const std::string& request);

} // namespace net

namespace std {
template<>
struct hash<net::Connection> {
    std::size_t operator()(const net::Connection& conn) const noexcept {
        return std::hash<int>()(conn.client_sock) ^ std::hash<int>()(conn.server_sock);
    }
};
} // namespace std