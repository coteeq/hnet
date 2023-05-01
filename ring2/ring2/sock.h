#pragma once
#include "addr.h"

namespace net {

class UdpSocket {
public:
    UdpSocket();
    explicit UdpSocket(int fd);
    ~UdpSocket();

    int fd() const;
    void bind(Addr addr);

private:
    int fd_;
};

}
