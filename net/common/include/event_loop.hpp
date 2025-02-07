#pragma once

#include "defines.hpp"
#include <cstdint>
#include <functional>
#include <memory>
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

class Event {
public:
    NET_DECLARE_PTRS(Event)

    Event(int fd, const std::shared_ptr<EventHandler>& handler): m_fd(fd), m_handler(handler) {
        if (m_handler->m_on_read) {
            m_type = static_cast<EventType>(static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::READ));
        }
        if (m_handler->m_on_write) {
            m_type = static_cast<EventType>(static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::WRITE));
        }
        if (m_handler->m_on_error) {
            m_type = static_cast<EventType>(
                static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::ERROR)
                | static_cast<uint8_t>(EventType::HUP)
            );
        }
    }

    virtual ~Event() = default;

    EventType get_type() {
        return m_type;
    }

    int get_fd() const {
        return m_fd;
    }

    void on_read() {
        if (m_handler->m_on_read) {
            m_handler->m_on_read(m_fd);
        }
    }

    void on_write() {
        if (m_handler->m_on_write) {
            m_handler->m_on_write(m_fd);
        }
    }

    void on_error() {
        if (m_handler->m_on_error) {
            m_handler->m_on_error(m_fd);
        }
    }

    void on_trigger() {
        if (static_cast<uint8_t>(m_type) & static_cast<uint8_t>(EventType::READ)) {
            on_read();
        }
        if (static_cast<uint8_t>(m_type) & static_cast<uint8_t>(EventType::WRITE)) {
            on_write();
        }
        if (static_cast<uint8_t>(m_type) & static_cast<uint8_t>(EventType::ERROR)) {
            on_error();
        }
    }

private:
    EventType m_type;
    int m_fd;
    std::shared_ptr<EventHandler> m_handler;
};

class EventLoop {
public:
    NET_DECLARE_PTRS(EventLoop)

    virtual ~EventLoop() = default;

    virtual void add_event(const std::shared_ptr<Event>& event) = 0;

    virtual void remove_event(const std::shared_ptr<Event>& event) = 0;

    virtual void wait_for_events() = 0;
};

class SelectEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(SelectEventLoop)

    SelectEventLoop(): m_max_fd(0) {}

    void add_event(const std::shared_ptr<Event>& event) override {
        m_events[event->get_fd()] = event;
        m_max_fd = std::max(m_max_fd, event->get_fd());
        if (static_cast<uint8_t>(event->get_type()) & static_cast<uint8_t>(EventType::READ)) {
            FD_SET(event->get_fd(), &read_fds);
        }
        if (static_cast<uint8_t>(event->get_type()) & static_cast<uint8_t>(EventType::WRITE)) {
            FD_SET(event->get_fd(), &write_fds);
        }
        if (static_cast<uint8_t>(event->get_type()) & static_cast<uint8_t>(EventType::ERROR)
            || static_cast<uint8_t>(event->get_type()) & static_cast<uint8_t>(EventType::HUP))
        {
            FD_SET(event->get_fd(), &error_fds);
        }
    }

    void remove_event(const std::shared_ptr<Event>& event) override {
        FD_CLR(event->get_fd(), &read_fds);
        FD_CLR(event->get_fd(), &write_fds);
        FD_CLR(event->get_fd(), &error_fds);

        m_events.erase(event->get_fd());
    }

    void wait_for_events() override {
        fd_set temp_read_fds = read_fds;
        fd_set temp_write_fds = write_fds;
        fd_set temp_error_fds = error_fds;

        int result = select(m_max_fd + 1, &temp_read_fds, &temp_write_fds, &temp_error_fds, nullptr);
        if (result < 0) {
            throw std::runtime_error("select failed");
        }

        // 遍历已触发的事件
        for (const auto& [fd, event]: m_events) {
            if (FD_ISSET(fd, &temp_read_fds)) {
                event->on_read();
            }
            if (FD_ISSET(fd, &temp_write_fds)) {
                event->on_write();
            }
            if (FD_ISSET(fd, &temp_error_fds)) {
                event->on_error();
            }
        }
    }

private:
    fd_set read_fds, write_fds, error_fds;
    std::unordered_map<int, std::shared_ptr<Event>> m_events;
    int m_max_fd;
};

class PollEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(PollEventLoop)

    void add_event(const std::shared_ptr<Event>& event) override {
        m_poll_fds.push_back({ event->get_fd(), 0, 0 });
        m_events[event->get_fd()] = event;
    }

    void remove_event(const std::shared_ptr<Event>& event) override {
        m_poll_fds.erase(
            std::remove_if(
                m_poll_fds.begin(),
                m_poll_fds.end(),
                [event](const pollfd& p) { return p.fd == event->get_fd(); }
            ),
            m_poll_fds.end()
        );
        m_events.erase(event->get_fd());
    }

    void wait_for_events() override {
        int result = poll(m_poll_fds.data(), m_poll_fds.size(), -1);
        if (result < 0) {
            throw std::runtime_error("poll failed");
        }

        for (size_t i = 0; i < m_poll_fds.size(); ++i) {
            if (m_poll_fds[i].revents != 0) {
                // Trigger event based on revents
                // Handle accordingly, like using m_events[i]->trigger()
                if (m_poll_fds[i].revents & POLLIN) {
                    m_events.at(m_poll_fds[i].fd)->on_read();
                }
                if (m_poll_fds[i].revents & POLLOUT) {
                    m_events.at(m_poll_fds[i].fd)->on_write();
                }
                if (m_poll_fds[i].revents & (POLLERR | POLLHUP)) {
                    m_events.at(m_poll_fds[i].fd)->on_error();
                }
            }
        }
    }

private:
    std::vector<pollfd> m_poll_fds;
    std::unordered_map<int, std::shared_ptr<Event>> m_events;
};

class EpollEventLoop: public EventLoop {
public:
    NET_DECLARE_PTRS(EpollEventLoop)

    EpollEventLoop(): m_epoll_fd(epoll_create1(0)) {
        if (m_epoll_fd == -1) {
            throw std::runtime_error("Failed to create epoll instance");
        }
    }

    ~EpollEventLoop() {
        close(m_epoll_fd);
    }

    void add_event(const std::shared_ptr<Event>& event) override {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = event->get_fd();
        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, event->get_fd(), &ev) == -1) {
            throw std::runtime_error("Failed to add event to epoll");
        }
        m_events[event->get_fd()] = event;
    }

    void remove_event(const std::shared_ptr<Event>& event) override {
        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event->get_fd(), nullptr) == -1) {
            throw std::runtime_error("Failed to remove event from epoll");
        }
        m_events.erase(event->get_fd());
    }

    void wait_for_events() override {
        std::vector<struct epoll_event> events(10);
        int num_events = epoll_wait(m_epoll_fd, events.data(), events.size(), -1);
        if (num_events < 0) {
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].events & EPOLLIN) {
                m_events.at(events[i].data.fd)->on_read();
            }
            if (events[i].events & EPOLLOUT) {
                m_events.at(events[i].data.fd)->on_write();
            }
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                m_events.at(events[i].data.fd)->on_error();
            }
        }
    }

private:
    int m_epoll_fd;
    std::unordered_map<int, std::shared_ptr<Event>> m_events;
};

} // namespace net