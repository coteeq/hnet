#pragma once
#include "addr.h"

namespace net {

class Socket {
public:
    Socket(IPFamily, Proto);
    explicit Socket(int fd);
    ~Socket();

    inline int fd() const {
        return fd_;
    }
    void bind(Addr addr);
    void listen(int n);

private:
    int fd_;
};

}
