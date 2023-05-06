#pragma once
#include "ring.h"
#include "poller.h"
#include "addr.h"

namespace net {

struct MsgHdr {
    int raw_ret;
    std::string buf; // FIXME
    Addr addr;
};

class Submitter {
public:
    Submitter(std::shared_ptr<Ring> ring);

    MsgHdr recvmsg(int fd) const;
    int sendmsg(int fd, MsgHdr& hdr) const;
    std::pair<int, Addr> accept(int fd) const;

private:
    std::shared_ptr<Ring> ring_;
};

}
