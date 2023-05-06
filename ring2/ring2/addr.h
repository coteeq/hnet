#pragma once

#include <arpa/inet.h>
#include <string>
#include <fmt/core.h>

namespace net {

enum class IPFamily {
    V4 = 0,
    V6 = 1,
};

enum class Proto {
    TCP = 0,
    UDP = 1,
};

struct Addr {
    Addr();

    union {
        struct sockaddr addr_;
        struct sockaddr_in addr4_;
        struct sockaddr_in6 addr6_;
    };
    socklen_t addrlen_;

    static Addr ip6_any(uint16_t port);
    static Addr ip6_local(uint16_t port);
    static Addr ip4_any(uint16_t port);
    static Addr ip4_local(uint16_t port);
    static Addr from_parts(const std::string& addr, uint16_t port, IPFamily family);
    static Addr from_addr_in6(const struct sockaddr_in6& addr_in6);

    std::string to_string() const;
};

}

template <> struct fmt::formatter<net::Addr>: formatter<std::string> {
    template <typename FormatContext>
    auto format(net::Addr addr, FormatContext& ctx) const {
        return formatter<std::string>::format(addr.to_string(), ctx);
    }
};
