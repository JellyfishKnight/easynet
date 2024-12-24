#pragma once

namespace net {


enum class SocketStatus {
    CLOSED = 0,
    CONNECTED,
    LISTENING,
    ACCEPTED
};


} // namespace net


