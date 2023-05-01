#include "sock.h"

#include <arpa/inet.h>
#include <fallible/error/make.hpp>
#include <unistd.h>

namespace net {

UdpSocket::UdpSocket(int fd)
    : fd_(fd)
{}

UdpSocket::UdpSocket()
    : fd_(-1)
{
    fd_ = socket(AF_INET6, SOCK_DGRAM, 0);
    SYSCALL_VERIFY(fd_ >= 0, "socket");
}

int UdpSocket::fd() const {
    return fd_;
}

UdpSocket::~UdpSocket() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

void UdpSocket::bind(Addr addr) {
    int err = ::bind(fd_, (struct sockaddr *)&addr.addr_, addr.addrlen_);
    SYSCALL_VERIFY(err == 0, "bind");
}

}
