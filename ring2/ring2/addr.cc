#include "addr.h"
#include <cstring>
#include <fallible/error/make.hpp>
#include <sys/socket.h>

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

Addr Addr::from_parts(const std::string& address, uint16_t port) {
    Addr addr;
    auto server_address = reinterpret_cast<sockaddr_in6*>(&addr.addr_);
    memset(server_address, 0, sizeof(*server_address));
    server_address->sin6_family = AF_INET6;
    server_address->sin6_port = htons(port);
    addr.addrlen_ = sizeof(*server_address);

    SYSCALL_VERIFY(inet_pton(AF_INET6, address.c_str(), &server_address->sin6_addr) == 1, "inet_pton");
    return addr;
}

Addr Addr::from_addr_in6(const struct sockaddr_in6& addr_in6) {
    Addr addr;
    addr.addr6_ = addr_in6;
    addr.addrlen_ = sizeof(sockaddr_in6);
    return addr;
}

std::string Addr::to_string() const {
    char ip_str[INET6_ADDRSTRLEN];
    auto addr6 = reinterpret_cast<const sockaddr_in6*>(&addr_);
    inet_ntop(AF_INET6, &addr6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
    return ip_str;
}

}
