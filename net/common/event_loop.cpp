#include "event_loop.hpp"
#include "defines.hpp"
#include <format>
#include <stdexcept>
#include <sys/epoll.h>

namespace net {

Event::Event(int fd, const std::shared_ptr<EventHandler>& handler): m_fd(fd), m_handler(handler) {
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

EventType Event::get_type() const {
    return m_type;
}

int Event::get_fd() const {
    return m_fd;
}

void Event::on_read() {
    if (m_handler->m_on_read) {
        m_handler->m_on_read(m_fd);
    }
}

void Event::on_write() {
    if (m_handler->m_on_write) {
        m_handler->m_on_write(m_fd);
    }
}

void Event::on_error() {
    if (m_handler->m_on_error) {
        m_handler->m_on_error(m_fd);
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

SelectEventLoop::SelectEventLoop(int time_out): m_max_fd(0), time_out(time_out) {}

void SelectEventLoop::add_event(const std::shared_ptr<Event>& event) {
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

    m_events[event->get_fd()] = event;
}

void SelectEventLoop::remove_event(int event_fd) {
    FD_CLR(event_fd, &read_fds);
    FD_CLR(event_fd, &write_fds);
    FD_CLR(event_fd, &error_fds);

    m_events.erase(event_fd);
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

PollEventLoop::PollEventLoop(int time_out): time_out(time_out) {}

void PollEventLoop::add_event(const std::shared_ptr<Event>& event) {
    m_poll_fds.push_back({ event->get_fd(), POLLIN | POLLOUT | POLLERR | POLLHUP, 0 });
    m_events[event->get_fd()] = event;
}

void PollEventLoop::remove_event(int event_fd) {
    for (size_t i = 0; i < m_poll_fds.size(); ++i) {
        if (m_poll_fds[i].fd == event_fd) {
            m_poll_fds.erase(m_poll_fds.begin() + i);
            break;
        }
    }

    m_events.erase(event_fd);
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
    ev.data.fd = event->get_fd();
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, event->get_fd(), &ev) == -1) {
        throw std::runtime_error("Failed to add event to epoll");
    }
    m_events[event->get_fd()] = event;
}

void EpollEventLoop::remove_event(int event_fd) {
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr) == -1) {
        throw std::runtime_error("Failed to remove event from epoll");
    }
    m_events.erase(event_fd);
}

void EpollEventLoop::wait_for_events() {
    std::vector<struct epoll_event> events(1024);

    int num_events = epoll_wait(m_epoll_fd, events.data(), events.size(), time_out);
    if (num_events < 0) {
        throw std::runtime_error(get_error_msg());
    }

    for (int i = 0; i < num_events; ++i) {
        try {
            if (events[i].events & EPOLLIN) {
                m_events.at(events[i].data.fd)->on_read();
            }
            if (events[i].events & EPOLLOUT) {
                m_events.at(events[i].data.fd)->on_write();
            }
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                m_events.at(events[i].data.fd)->on_error();
            }
        } catch (std::out_of_range& e) {
            // Ignore the event if it's not in the map
        }
    }
}

} // namespace net