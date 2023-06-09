#include "sock.h"

#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <fallible/error/make.hpp>
#include <unistd.h>
#include <netinet/tcp.h>

namespace net {

Socket::Socket(int fd)
    : fd_(fd)
{}

Socket::Socket(IPFamily family, Proto proto)
    : fd_(-1)
{
    int domain;
    switch (family) {
        case IPFamily::V4:
            domain = AF_INET;
            break;
        case IPFamily::V6:
            domain = AF_INET6;
            break;
    }

    int type;
    switch (proto) {
        case Proto::TCP:
            type = SOCK_STREAM;
            break;
        case Proto::UDP:
            type = SOCK_DGRAM;
            break;
    }
    fd_ = socket(domain, type, 0);
    SYSCALL_VERIFY(fd_ >= 0, "socket");

    if (proto == Proto::TCP) {
        int val = 1;
        SYSCALL_VERIFY(setsockopt(fd_, IPPROTO_TCP, TCP_FASTOPEN, &val, sizeof(val)) == 0, "setsockopt");
        SYSCALL_VERIFY(setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == 0, "setsockopt");
    }
}

Socket::~Socket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void Socket::bind(Addr addr) {
    int err = ::bind(fd_, &addr.addr_, addr.addrlen_);
    SYSCALL_VERIFY(err == 0, "bind");
}

void Socket::listen(int n) {
    int err = ::listen(fd(), n);
    SYSCALL_VERIFY(err != -1, "listen");
}

}
