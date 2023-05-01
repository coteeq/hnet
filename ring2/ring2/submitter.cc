#include "submitter.h"
#include <tf/sched/yield.hpp>
#include <tf/rt/scheduler.hpp>
#include <liburing.h>


namespace net {


Submitter::Submitter(std::shared_ptr<Ring> ring)
    : ring_(ring)
{}

MsgHdr Submitter::recvmsg(int fd) {
    struct msghdr res_hdr;
    MsgHdr result;

    struct iovec buf;
    buf.iov_base = malloc(buf.iov_len = 500);
    res_hdr.msg_iov = &buf;
    res_hdr.msg_iovlen = 1;
    res_hdr.msg_name = &result.addr.addr_;
    res_hdr.msg_namelen = result.addr.addrlen_;

    auto* sched = tf::rt::Scheduler::Current();
    auto res_setter = UringResSetter {
        .fiber = sched->RunningFiber(),
        .res = 0,
    };

    ring_->submit([&] (struct io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, &res_hdr, 0);
        io_uring_sqe_set_data(sqe, &res_setter);
    });
    sched->Suspend();

    result.buf = std::string((char*)buf.iov_base, res_setter.res);
    free(buf.iov_base);
    return result;
}

// int Submitter::sendmsg(int fd, MsgHdr hdr) {

// }


}
