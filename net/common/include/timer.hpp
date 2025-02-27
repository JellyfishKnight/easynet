#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
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

    void start_timing() {
        assert(m_timeout_set && "Timeout not set");
        m_status = TimerStatus::RUNNING;
        auto time_has_passed = std::chrono::nanoseconds(0);
        auto start_time = std::chrono::high_resolution_clock::now();
        while (time_has_passed < m_timeout) {
            if (m_status == TimerStatus::PAUSED || m_status == TimerStatus::STOPPED) {
                break;
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            time_has_passed = end_time - start_time;
        }
        m_timeout_flag = true;
        m_status = TimerStatus::STOPPED;
        if (m_timeout_action) {
            m_timeout_action();
        }
    }

    void start_interval() {
        m_status = TimerStatus::RUNNING;
        while (m_status == TimerStatus::RUNNING) {
            sleep();
            if (m_interval_action) {
                m_interval_action();
            }
        }
    }

    void async_start_timing() {
        auto unused_future = std::async(std::launch::async, [this]() { start_timing(); });
    }

    void async_start_interval() {
        auto unused_future = std::async(std::launch::async, [this]() { start_interval(); });
    }

    TimerStatus status() const {
        return m_status;
    }

    bool timeout() const {
        return m_timeout_flag;
    }

    void on_time_out(std::function<void()> action) {
        m_timeout_action = action;
    }

    void on_time_interval(std::function<void()> action) {
        m_interval_action = action;
    }

    void pause() {
        m_status = TimerStatus::PAUSED;
    }

    void resume() {
        m_status = TimerStatus::RUNNING;
    }

    void stop() {
        m_status = TimerStatus::STOPPED;
    }

    void reset() {
        m_timeout_flag = false;
    }

private:
    std::chrono::nanoseconds m_interval;
    std::chrono::nanoseconds m_timeout;

    bool m_timeout_flag = false;
    bool m_timeout_set = false;
    std::function<void()> m_timeout_action;
    std::function<void()> m_interval_action;

    TimerStatus m_status = TimerStatus::STOPPED;
};

} // namespace net