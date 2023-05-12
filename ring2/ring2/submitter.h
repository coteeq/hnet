#pragma once
#include "ring.h"
#include "poller.h"
#include "addr.h"
#include <wheels/memory/view.hpp>
#include <five/time.h>

namespace net {

const int ENOCQE = 100512;
const int ESQE_TIMEOUT = 100513;

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
    ReadView recvmsg(int fd, wheels::MutableMemView buf, five::Duration dur) const;
    int sendmsg(int fd, MsgHdr& hdr) const;
    int sendmsg(int fd, WriteView& hdr) const;
    std::pair<int, Addr> accept(int fd) const;

private:
    std::shared_ptr<Ring> ring_;
};

}
