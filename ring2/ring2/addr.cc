#include "addr.h"
#include <cstring>
#include <fallible/error/make.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <wheels/core/panic.hpp>

namespace {

net::Addr from_in6_addr(struct in6_addr in6, uint16_t port) {
    net::Addr addr;

    auto* sa = reinterpret_cast<struct sockaddr_in6*>(&addr.addr_);
    memset(sa, 0, sizeof(*sa));
    sa->sin6_addr = in6;
    sa->sin6_port = htons(port);
    sa->sin6_family = AF_INET6;

    addr.addrlen_ = sizeof(*sa);
    return addr;
}

}

namespace net {

Addr::Addr()
    : addrlen_(sizeof(struct sockaddr_in6))
{}

Addr Addr::ip6_any(uint16_t port) {
    return from_in6_addr(in6addr_any, port);
}

Addr Addr::ip6_local(uint16_t port) {
    return from_in6_addr(in6addr_loopback, port);
}


Addr Addr::ip4_any(uint16_t port) {
    Addr addr;
    memset(&addr.addr4_, 0, sizeof(addr.addr4_));
    addr.addr4_.sin_family = AF_INET;
    addr.addr4_.sin_port = htons(port);
    addr.addr4_.sin_addr.s_addr = INADDR_ANY;
    addr.addrlen_ = sizeof(addr.addr4_);
    return addr;
}

Addr Addr::ip4_local(uint16_t port) {
    Addr addr;
    memset(&addr.addr4_, 0, sizeof(addr.addr4_));
    addr.addr4_.sin_family = AF_INET;
    addr.addr4_.sin_port = htons(port);
    addr.addr4_.sin_addr.s_addr = INADDR_LOOPBACK;
    addr.addrlen_ = sizeof(addr.addr4_);
    return addr;
}


Addr Addr::from_parts(const std::string& address, uint16_t port, IPFamily family) {
    Addr addr;
    switch (family) {
        case IPFamily::V4: {
            auto* server_address = &addr.addr4_;
            memset(server_address, 0, sizeof(*server_address));
            server_address->sin_family = AF_INET;
            server_address->sin_port = htons(port);
            addr.addrlen_ = sizeof(*server_address);

            SYSCALL_VERIFY(inet_pton(AF_INET, address.c_str(), &server_address->sin_addr) == 1, "inet_pton");
            break;
        }
        case IPFamily::V6: {
            auto* server_address = &addr.addr6_;
            memset(server_address, 0, sizeof(*server_address));
            server_address->sin6_family = AF_INET6;
            server_address->sin6_port = htons(port);
            addr.addrlen_ = sizeof(*server_address);

            SYSCALL_VERIFY(inet_pton(AF_INET6, address.c_str(), &server_address->sin6_addr) == 1, "inet_pton");
            break;
        }
    }
    return addr;
}

Addr Addr::from_addr_in6(const struct sockaddr_in6& addr_in6) {
    Addr addr;
    addr.addr6_ = addr_in6;
    addr.addrlen_ = sizeof(sockaddr_in6);
    return addr;
}

std::string Addr::to_string() const {
    constexpr int len = std::max(INET6_ADDRSTRLEN, INET_ADDRSTRLEN);
    char ip_str[len];
    switch (addr_.sa_family) {
        case AF_UNSPEC: {
            return "(null)";
        }
        case AF_INET: {
            SYSCALL_VERIFY(inet_ntop(AF_INET, &addr4_.sin_addr, ip_str, len) != nullptr, "inet_ntop");
            return fmt::format("{}:{}", ip_str, ntohs(addr4_.sin_port));
        }
        case AF_INET6: {
            SYSCALL_VERIFY(inet_ntop(AF_INET6, &addr6_.sin6_addr, ip_str, len) != nullptr, "inet_ntop");
            return fmt::format("[{}]:{}", ip_str, ntohs(addr6_.sin6_port));
        }
        default: {
            WHEELS_PANIC("unreachable");
        }
    }
}

}
