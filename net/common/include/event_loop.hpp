#pragma once

#include "defines.hpp"
#include "remote_target.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <unordered_map>

namespace net {

enum class EventLoopType : uint8_t { SELECT = 0x01, POLL = 0x02, EPOLL = 0x03 };

enum class EventType : uint8_t { READ = 0x01, WRITE = 0x02, ERROR = 0x04, HUP = 0x08 };

struct EventHandler {
    NET_DECLARE_PTRS(EventHandler)

    using Callback = std::function<void(int)>;

    Callback m_on_read = nullptr;
    Callback m_on_write = nullptr;
    Callback m_on_error = nullptr;
};

class Event: public RemoteTarget {
public:
    NET_DECLARE_PTRS(Event)

    Event(int fd, const std::shared_ptr<EventHandler>& handler);

    virtual ~Event() = default;

    EventType type() const;

    void on_read();

    void on_write();

    void on_error();

    void on_trigger();

private:
    EventType m_type;
    std::shared_ptr<EventHandler> m_handler;
};

class EventLoop {
public:
    NET_DECLARE_PTRS(EventLoop)

    virtual ~EventLoop() = default;

    virtual void add_event(const std::shared_ptr<Event>& event) = 0;

    virtual void remove_event(int event_fd) = 0;

    virtual void wait_for_events() = 0;

    std::shared_ptr<Event> get_event(int event_fd);

protected:
    RemotePool m_remote_pool;
};

/**
 * @brief Event loop implementation using select system call
 *        But this is NOT IN USE now, cause when socket value comes to
 *        1024, it will cause error, need further investigation
 */
class SelectEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(SelectEventLoop)

    SelectEventLoop(int time_out = -1);

    void add_event(const std::shared_ptr<Event>& event) override;

    void remove_event(int event_fd) override;

    void wait_for_events() override;

private:
    fd_set read_fds, write_fds, error_fds;
    int m_max_fd;
    int time_out;
};

class PollEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(PollEventLoop)

    PollEventLoop(int time_out = -1);

    void add_event(const std::shared_ptr<Event>& event) override;

    void remove_event(int event_fd) override;

    void wait_for_events() override;

private:
    std::vector<pollfd> m_poll_fds;
    int time_out;
};

class EpollEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(EpollEventLoop)

    EpollEventLoop(int time_out = -1);

    ~EpollEventLoop();

    void add_event(const std::shared_ptr<Event>& event) override;

    void remove_event(int event_fd) override;

    void wait_for_events() override;

private:
    int m_epoll_fd;
    int time_out;
};

} // namespace net