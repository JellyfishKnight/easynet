#pragma once

#include <cstdint>
#include <system_error>

namespace net {

#define NET_DECLARE_PTRS(CLASS) \
    using SharedPtr = std::shared_ptr<CLASS>; \
    using ConstSharedPtr = std::shared_ptr<const CLASS>; \
    using WeakPtr = std::weak_ptr<CLASS>; \
    using ConstWeakPtr = std::weak_ptr<const CLASS>; \
    using UniquePtr = std::unique_ptr<CLASS>; \
    using ConstUniquePtr = std::unique_ptr<const CLASS>;

struct NetError {
    int error_code;
    std::string msg;
};

#define NET_CONNECTION_RESET_CODE 0
#define NET_TIMEOUT_CODE 1
#define NET_INVALID_EVENT_LOOP_CODE 2
#define NET_WEBSOCKET_PARSE_WANT_READ 3
#define NET_INVALID_WEBSOCKET_UPGRADE_CODE 4
#define NET_HTTP_PARSE_WANT_READ 5
#define NET_EARLY_END_OF_SOCKET 6
#define NET_NO_CLIENT_FOUND 7
#define NET_CLIENT_ALREADY_EXISTS 8
#define NET_HTTP_PROXY_INVALID_URL 9


#define GET_ERROR_MSG() \
    NetError { errno, std::system_category().message(errno) }

} // namespace net