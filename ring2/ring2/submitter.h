#pragma once
#include "ring.h"
#include "poller.h"
#include "addr.h"
#include "wheels/memory/view.hpp"

namespace net {

struct MsgHdr {
    int raw_ret;
    std::string buf; // FIXME
    Addr addr;
};

struct ReadView {
    int ret;
    wheels::MutableMemView buf;
    Addr addr;
};

struct WriteView {
    wheels::ConstMemView buf;
    Addr addr;
};

class Submitter {
public:
    Submitter(std::shared_ptr<Ring> ring);

    MsgHdr recvmsg(int fd) const;
    ReadView recvmsg(int fd, wheels::MutableMemView buf) const;
    int sendmsg(int fd, MsgHdr& hdr) const;
    int sendmsg(int fd, WriteView& hdr) const;
    std::pair<int, Addr> accept(int fd) const;

private:
    std::shared_ptr<Ring> ring_;
};

}
