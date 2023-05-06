#pragma once
#include "addr.h"

namespace net {

class Socket {
public:
    Socket(IPFamily, Proto);
    explicit Socket(int fd);
    ~Socket();

    int fd() const;
    void bind(Addr addr);
    void listen(int n);

private:
    int fd_;
};

}
