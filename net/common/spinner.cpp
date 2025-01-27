#include "spinner.hpp"

namespace net {

void ConditionSpinner::signal_exit() {
    std::lock_guard<std::mutex> lock(mtx);
    running = false;
    cv.notify_one();
}

void ConditionSpinner::wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !running; });
}

} // namespace net