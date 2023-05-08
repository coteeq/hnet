#include "submitter.h"
#include "five/time.h"
#include "wheels/core/panic.hpp"
#include "wheels/logging/logging.hpp"
#include "wheels/memory/view.hpp"
#include <fallible/error/make.hpp>
#include <tf/sched/yield.hpp>
#include <tf/rt/scheduler.hpp>
#include <liburing.h>


namespace net {

const int ENOCQE = 100512;

int ring_submit(std::shared_ptr<Ring> ring, Ring::RequestBuilder builder) {
    auto* sched = tf::rt::Scheduler::Current();
    auto res_setter = UringResSetter {
        .fiber = sched->RunningFiber(),
        .res = -ENOCQE,
    };

    ring->submit([&] (struct io_uring_sqe* sqe) {
        builder(sqe);
        io_uring_sqe_set_data(sqe, &res_setter);
    });

    sched->Suspend();

    if (res_setter.res == -ENOCQE) {
        WHEELS_PANIC("enocqe");
    }

    return res_setter.res;
}


Submitter::Submitter(std::shared_ptr<Ring> ring)
    : ring_(ring)
{}

MsgHdr Submitter::recvmsg(int fd) const {
    struct msghdr res_hdr = {};
    MsgHdr result;

    char data[65'000];
    struct iovec buf;
    buf.iov_len = 65'000;
    // buf.iov_base = malloc(buf.iov_len);
    buf.iov_base = data;
    res_hdr.msg_iov = &buf;
    res_hdr.msg_iovlen = 1;
    res_hdr.msg_name = &result.addr.addr_;
    res_hdr.msg_namelen = result.addr.addrlen_;

    auto res = ring_submit(ring_, [&] (struct io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, &res_hdr, 0);
    });

    // if (res < 0) {
    //     fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}, WHEELS_HERE));
    // }
    if (res >= 0) {
        result.buf = std::string((char*)buf.iov_base, res);
    }
    result.raw_ret = res;
    // free(buf.iov_base);
    return result;
}

ReadView Submitter::recvmsg(int fd, wheels::MutableMemView buf) const {
    struct msghdr res_hdr = {};
    ReadView result;

    struct iovec iov;
    iov.iov_len = buf.Size();
    iov.iov_base = buf.Begin();
    res_hdr.msg_iov = &iov;
    res_hdr.msg_iovlen = 1;
    res_hdr.msg_name = &result.addr.addr_;
    res_hdr.msg_namelen = result.addr.addrlen_;

    auto res = ring_submit(ring_, [&] (struct io_uring_sqe* sqe) {
        io_uring_prep_recvmsg(sqe, fd, &res_hdr, 0);
    });

    if (res >= 0) {
        result.buf = wheels::MutableMemView((char*)iov.iov_base, res);
    }
    result.ret = res;
    return result;
}

int Submitter::sendmsg(int fd, MsgHdr& hdr) const {
    struct msghdr res_hdr = {};

    struct iovec buf;
    buf.iov_base = (void*)hdr.buf.data();
    buf.iov_len = hdr.buf.size();
    res_hdr.msg_iov = &buf;
    res_hdr.msg_iovlen = 1;
    res_hdr.msg_name = &hdr.addr.addr_;
    res_hdr.msg_namelen = hdr.addr.addrlen_;

    auto res = ring_submit(ring_, [&] (struct io_uring_sqe* sqe) {
        io_uring_prep_sendmsg(sqe, fd, &res_hdr, 0);
    });

    if (res < 0) {
        fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}, WHEELS_HERE));
    }

    return res;
}

int Submitter::sendmsg(int fd, WriteView& ww) const {
    struct msghdr res_hdr = {};

    struct iovec buf;
    buf.iov_base = (void*)ww.buf.Begin();
    buf.iov_len = ww.buf.Size();
    res_hdr.msg_iov = &buf;
    res_hdr.msg_iovlen = 1;
    res_hdr.msg_name = &ww.addr.addr_;
    res_hdr.msg_namelen = ww.addr.addrlen_;

    auto res = ring_submit(ring_, [&] (struct io_uring_sqe* sqe) {
        io_uring_prep_sendmsg(sqe, fd, &res_hdr, 0);
    });

    if (res < 0) {
        fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}, WHEELS_HERE));
    }

    return res;
}

std::pair<int, Addr> Submitter::accept(int fd) const {
    Addr addr;
    auto res = ring_submit(ring_, [&] (struct io_uring_sqe* sqe) {
        io_uring_prep_accept(sqe, fd, &addr.addr_, &addr.addrlen_, 0);
    });

    if (res < 0) {
        fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}, WHEELS_HERE));
    }

    return std::make_pair(res, addr);
}


}
