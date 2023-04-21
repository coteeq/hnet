#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <memory>
#include <fmt/format.h>
#include <sys/socket.h>


namespace net {

struct Addr {
    union {
        struct sockaddr addr_;
        struct sockaddr_in6 addr6_;
    };
    socklen_t addrlen_;

    static Addr ip6_any(uint16_t port);
    static Addr ip6_local(uint16_t port);
    static Addr from_parts(const std::string& addr, uint16_t port);
    static Addr from_addr_in6(const struct sockaddr_in6& addr_in6);

    std::string to_string() const;
};

class Socket {
public:
    // @brief bind to localhost
    virtual void bind(const Addr& addr) = 0;

    virtual int fd() = 0;

    virtual ~Socket() {}
};


std::shared_ptr<Socket> make_socket();


}

template <> struct fmt::formatter<net::Addr>: formatter<string_view> {
    template <typename FormatContext>
    auto format(net::Addr addr, FormatContext& ctx) const {
        const int port_len = 5;
        const int colon_len = 1;
        const int terminator_len = 1;
        char buf[INET6_ADDRSTRLEN + colon_len + port_len + terminator_len];
        inet_ntop(AF_INET6, &addr.addr6_, buf, addr.addrlen_);
        auto addr_str = fmt::format("{}:{}", buf, ntohs(addr.addr6_.sin6_port));
        return formatter<string_view>::format(string_view(addr_str), ctx);
    }
};
