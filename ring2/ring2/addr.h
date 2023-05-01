#pragma once

#include <arpa/inet.h>
#include <string>

namespace net {

struct Addr {
    Addr();

    union {
        struct sockaddr addr_;
        struct sockaddr_in6 addr6_;
        struct sockaddr_in addr4_;
    };
    socklen_t addrlen_;

    static Addr ip6_any(uint16_t port);
    static Addr ip6_local(uint16_t port);
    static Addr from_parts(const std::string& addr, uint16_t port);
    static Addr from_addr_in6(const struct sockaddr_in6& addr_in6);

    std::string to_string() const;
};

}
