#pragma once

namespace net {


enum class SocketStatus : int {
    CLOSED = 0,
    CONNECTED,
    LISTENING,
    ACCEPTED
};


} // namespace net


