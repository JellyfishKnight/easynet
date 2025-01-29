#include "spinner.hpp"
#include <mutex>

namespace net {

void ConditionSpinner::signal_exit() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_running = false;
    m_cv.notify_one();
}

void ConditionSpinner::wait() {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.wait(lock, [this] { return !m_running; });
}

void LoopSpinner::signal_exit() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_running = false;
    m_timer.stop();
}

void LoopSpinner::wait() {
    while (m_running) {
        m_timer.start_interval();
    }
}

void LoopSpinner::set_timer(Timer&& timer) {
    this->m_timer = std::move(timer);
}

} // namespace net