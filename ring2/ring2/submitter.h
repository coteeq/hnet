#pragma once
#include "ring.h"
#include "poller.h"
#include "addr.h"

namespace net {

struct MsgHdr {
    std::string buf; // FIXME
    Addr addr;
};

class Submitter {
public:
    Submitter(std::shared_ptr<Ring> ring);

    MsgHdr recvmsg(int fd);
    int sendmsg(int fd, MsgHdr hdr);

private:
    std::shared_ptr<Ring> ring_;
};

}
