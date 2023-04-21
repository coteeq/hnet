#include "net.h"
#include <fallible/error/make.hpp>
#include <fallible/error/throw.hpp>
#include <wheels/core/compiler.hpp>

#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

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


class UdpSocket : public Socket {
public:
    explicit UdpSocket(int fd)
        : fd_(fd)
    {}

    UdpSocket()
        : fd_(-1)
    {
        fd_ = socket(AF_INET6, SOCK_DGRAM, 0);
        SYSCALL_VERIFY(fd_ >= 0, "socket");
    }

    int fd() override {
        return fd_;
    }

    void bind(const Addr& addr) override {
        int err = ::bind(fd_, (struct sockaddr *)&addr.addr_, addr.addrlen_);
        SYSCALL_VERIFY(err == 0, "bind");
    }

    ~UdpSocket() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

private:
    int fd_;
};


std::shared_ptr<Socket> make_socket() {
    return std::make_shared<UdpSocket>();
}

}
