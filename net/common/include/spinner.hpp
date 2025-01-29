#pragma once

#include "timer.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>

namespace net {

class Spinner {
public:
    virtual void signal_exit() = 0;

    virtual void wait() = 0;
};

class ConditionSpinner: public Spinner {
public:
    void signal_exit() override;

    void wait() override;

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_running = true;
};

class LoopSpinner: public Spinner {
public:
    void signal_exit() override;

    void wait() override;

    void set_timer(Timer&& timer);

    void on_interval(std::function<void()> action);

private:
    std::mutex m_mtx;
    bool m_running = true;
    Timer m_timer;
};

} // namespace net
