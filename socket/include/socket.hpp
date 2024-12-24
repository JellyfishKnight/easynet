#pragma once

#include "tcp.hpp"
#include "udp.hpp"
#include "common.hpp"
#include <functional>
#include <thread>

namespace net {

template<typename Server, int frequency = 0, int waiting_queue_size = 1>
void start_server(Server& server, std::function<void(Server&)> callback) {
    try {
        server.listen(waiting_queue_size);
        if (frequency == 0) {
            std::thread([&server, callback]() {
                while (true) {
                    server.accept();
                    callback(server);
                }
            }).detach();
        } else {
            std::thread([&server, callback]() {
                while (true) {
                    server.accept();
                    callback(server);
                    std::this_thread::sleep_for(std::chrono::milliseconds(frequency));
                }
            }).detach();
        }
    } catch (const std::exception& e) {
        throw e;
    }
}


} // namespace net