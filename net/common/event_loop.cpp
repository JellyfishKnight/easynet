#include "event_loop.hpp"
#include "defines.hpp"
#include "remote_target.hpp"
#include <format>
#include <memory>
#include <stdexcept>
#include <sys/epoll.h>

namespace net {

Event::Event(int fd, const std::shared_ptr<EventHandler>& handler): RemoteTarget(fd), m_handler(handler) {
    if (m_handler->m_on_read) {
        m_type = static_cast<EventType>(static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::READ));
    }
    if (m_handler->m_on_write) {
        m_type = static_cast<EventType>(static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::WRITE));
    }
    if (m_handler->m_on_error) {
        m_type = static_cast<EventType>(
            static_cast<uint8_t>(m_type) | static_cast<uint8_t>(EventType::ERROR) | static_cast<uint8_t>(EventType::HUP)
        );
    }
}

EventType Event::type() const {
    return m_type;
}

void Event::on_read() {
    if (m_handler->m_on_read) {
        m_handler->m_on_read(m_client_fd);
    }
}

void Event::on_write() {
    if (m_handler->m_on_write) {
        m_handler->m_on_write(m_client_fd);
    }
}

void Event::on_error() {
    if (m_handler->m_on_error) {
        m_handler->m_on_error(m_client_fd);
    }
}

void Event::on_trigger() {
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

std::shared_ptr<Event> EventLoop::get_event(int event_fd) {
    return std::dynamic_pointer_cast<Event>(m_remote_pool.get_remote(event_fd));
}

SelectEventLoop::SelectEventLoop(int time_out): m_max_fd(0), time_out(time_out) {}

void SelectEventLoop::add_event(const std::shared_ptr<Event>& event) {
    m_max_fd = std::max(m_max_fd, event->fd());
    if (static_cast<uint8_t>(event->type()) & static_cast<uint8_t>(EventType::READ)) {
        FD_SET(event->fd(), &read_fds);
    }
    if (static_cast<uint8_t>(event->type()) & static_cast<uint8_t>(EventType::WRITE)) {
        FD_SET(event->fd(), &write_fds);
    }
    if (static_cast<uint8_t>(event->type()) & static_cast<uint8_t>(EventType::ERROR)
        || static_cast<uint8_t>(event->type()) & static_cast<uint8_t>(EventType::HUP))
    {
        FD_SET(event->fd(), &error_fds);
    }
    m_remote_pool.add_remote(event);
}

void SelectEventLoop::remove_event(int event_fd) {
    FD_CLR(event_fd, &read_fds);
    FD_CLR(event_fd, &write_fds);
    FD_CLR(event_fd, &error_fds);
    m_remote_pool.remove_remote(event_fd);
}

void SelectEventLoop::wait_for_events() {
    fd_set temp_read_fds = read_fds;
    fd_set temp_write_fds = write_fds;
    fd_set temp_error_fds = error_fds;

    struct timeval tv {
        .tv_sec = time_out / 1000, .tv_usec = (time_out % 1000) * 1000
    };

    int result = select(m_max_fd + 1, &temp_read_fds, &temp_write_fds, &temp_error_fds, &tv);
    if (result < 0) {
        throw std::runtime_error(get_error_msg());
    }

    m_remote_pool.iterate([&temp_read_fds, &temp_write_fds, &temp_error_fds](RemoteTarget::SharedPtr remote) {
        auto event = std::dynamic_pointer_cast<Event>(remote);
        if (FD_ISSET(event->fd(), &temp_read_fds)) {
            event->on_read();
        }
        if (FD_ISSET(event->fd(), &temp_write_fds)) {
            event->on_write();
        }
        if (FD_ISSET(event->fd(), &temp_error_fds)) {
            event->on_error();
        }
    });
}

PollEventLoop::PollEventLoop(int time_out): time_out(time_out) {}

void PollEventLoop::add_event(const std::shared_ptr<Event>& event) {
    m_poll_fds.push_back({ event->fd(), POLLIN | POLLOUT | POLLERR | POLLHUP, 0 });
    m_remote_pool.add_remote(event);
}

void PollEventLoop::remove_event(int event_fd) {
    for (size_t i = 0; i < m_poll_fds.size(); ++i) {
        if (m_poll_fds[i].fd == event_fd) {
            m_poll_fds.erase(m_poll_fds.begin() + i);
            break;
        }
    }

    m_remote_pool.remove_remote(event_fd);
}

void PollEventLoop::wait_for_events() {
    int result = ::poll(m_poll_fds.data(), m_poll_fds.size(), time_out);
    if (result < 0) {
        throw std::runtime_error(get_error_msg());
    }

    for (size_t i = 0; i < m_poll_fds.size(); ++i) {
        if (m_poll_fds[i].revents != 0) {
            // Trigger event based on revents
            // Handle accordingly, like using m_events[i]->trigger()
            auto event = get_event(m_poll_fds[i].fd);
            if (m_poll_fds[i].revents & POLLIN) {
                event->on_read();
            }
            if (m_poll_fds[i].revents & POLLOUT) {
                event->on_write();
            }
            if (m_poll_fds[i].revents & (POLLERR | POLLHUP)) {
                event->on_error();
            }
        }
    }
}

EpollEventLoop::EpollEventLoop(int time_out): m_epoll_fd(epoll_create1(0)), time_out(time_out) {
    if (m_epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

EpollEventLoop::~EpollEventLoop() {
    close(m_epoll_fd);
}

void EpollEventLoop::add_event(const std::shared_ptr<Event>& event) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET;
    ev.data.fd = event->fd();
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, event->fd(), &ev) == -1) {
        throw std::runtime_error("Failed to add event to epoll");
    }
    m_remote_pool.add_remote(event);
}

void EpollEventLoop::remove_event(int event_fd) {
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr) == -1) {
        throw std::runtime_error("Failed to remove event from epoll");
    }
    m_remote_pool.remove_remote(event_fd);
}

void EpollEventLoop::wait_for_events() {
    std::vector<struct epoll_event> events(1024);

    int num_events = epoll_wait(m_epoll_fd, events.data(), events.size(), time_out);
    if (num_events < 0) {
        throw std::runtime_error(get_error_msg());
    }

    for (int i = 0; i < num_events; ++i) {
        auto event = get_event(events[i].data.fd);
        if (events[i].events & EPOLLIN) {
            event->on_read();
        }
        if (events[i].events & EPOLLOUT) {
            event->on_write();
        }
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            event->on_error();
        }
    }
}

} // namespace net