#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <thread>
namespace net {

enum class TimerStatus { STOPPED, RUNNING, PAUSED };

class Timer {
public:
    Timer() = default;
    Timer(const Timer&) = delete;
    Timer(Timer&&) = default;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&) = default;

    virtual ~Timer() = default;

    template<typename Rep, typename Period>
    void set_interval(std::chrono::duration<Rep, Period> interval) {
        m_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(interval);
    }

    void set_rate(double rate) {
        m_interval = std::chrono::nanoseconds(static_cast<uint64_t>(1e9 / rate));
    }

    template<typename Rep, typename Period>
    void set_timeout(std::chrono::duration<Rep, Period> timeout) {
        m_timeout = std::chrono::duration_cast<std::chrono::nanoseconds>(timeout);
        m_timeout_set = true;
    }

    void sleep() const {
        std::this_thread::sleep_for(m_interval);
    }

    void start() {
        assert(m_timeout_set && "Timeout not set");
        m_counter_thread = std::thread([this]() {
            m_status = TimerStatus::RUNNING;
            auto time_has_passed = std::chrono::nanoseconds(0);
            auto start_time = std::chrono::high_resolution_clock::now();
            while (time_has_passed < m_timeout) {
                if (m_status == TimerStatus::PAUSED) {
                    break;
                }
                auto end_time = std::chrono::high_resolution_clock::now();
                time_has_passed = end_time - start_time;
            }
            m_timeout_flag = true;
            m_status = TimerStatus::STOPPED;
        });
    }

    TimerStatus status() const {
        return m_status;
    }

    bool timeout() const {
        return m_timeout_flag;
    }

private:
    std::chrono::nanoseconds m_interval;
    std::chrono::nanoseconds m_timeout;

    bool m_timeout_flag = true;
    bool m_timeout_set = false;

    std::thread m_counter_thread;

    TimerStatus m_status = TimerStatus::STOPPED;
};

} // namespace net