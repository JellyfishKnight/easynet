#pragma once

#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace net {

class addressResolver {
public:
    struct address_ref {
        struct ::sockaddr* m_addr;
        ::socklen_t& m_len;
    };

    struct address {
        union {
            struct ::sockaddr m_addr;
            struct ::sockaddr_storage m_addr_storage;
        };

        ::socklen_t m_addr_len = sizeof(struct sockaddr_storage);

        operator address_ref() {
            return { &m_addr, m_addr_len };
        }
    };

    struct address_info {
        struct ::addrinfo* m_curr = nullptr;

        address_ref get_address() const {
            return { m_curr->ai_addr, m_curr->ai_addrlen };
        }

        int create_socket() const {
            int socketfd = ::socket(m_curr->ai_family, m_curr->ai_socktype, m_curr->ai_protocol);
            if (socketfd == -1) {
                throw std::system_error(errno, std::system_category(), "Failed to create socket");
            }
            return socketfd;
        }

        [[nodiscard]] bool next_entry() {
            m_curr = m_curr->ai_next;
            if (m_curr == nullptr) {
                return false;
            }
            return true;
        }
    };

    address_info resolve(const std::string& name, const std::string& service) {
        int err = ::getaddrinfo(name.c_str(), service.c_str(), nullptr, &m_head);
        if (err != 0) {
            throw std::system_error(err, std::system_category(), "Failed to resolve address");
        }
        return { m_head };
    }

    addressResolver() = default;

    ~addressResolver() {
        if (m_head != nullptr) {
            ::freeaddrinfo(m_head);
        }
    }

private:
    struct addrinfo* m_head = nullptr;
};

} // namespace net