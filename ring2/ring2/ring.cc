#include "ring.h"
#include <fallible/error/throw.hpp>
#include <liburing/io_uring.h>

#include <fallible/error/make.hpp>
#include <wheels/logging/logging.hpp>


namespace net {

UsefulCqe::UsefulCqe(const struct io_uring_cqe* cqe)
    : user_data(io_uring_cqe_get_data(cqe))
    , res(cqe->res)
{}

Ring::Ring(u32 entries, u32 cpuid) {
    struct io_uring_params params = {};
    params.flags |= IORING_SETUP_SQPOLL | IORING_SETUP_SINGLE_ISSUER;
    params.sq_thread_idle = 2000;

    if (cpuid != (u32)-1) {
        params.sq_thread_cpu = cpuid;
        params.flags |= IORING_SETUP_SQ_AFF;
    }

    if (int res = io_uring_queue_init_params(entries, &ring_, &params); res != 0) {
        fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}, "bad init"));
    }
    pending_count_ = 0;
}

int Ring::submit(RequestBuilder builder, bool next_is_timeout) const {
    struct io_uring_sqe* sqe = nullptr;
    bool starved = false;
    while (!sqe) {
        sqe = io_uring_get_sqe(&ring_);
        if (!sqe) {
            starved = true;
            starvation_.cycles++;
        }
    }
    if (starved) {
        starvation_.times++;
        if (starvation_.times % 1000 == 0) {
            LOG_WARN("starved total {} times / {} cycles", starvation_.times, starvation_.cycles);
        }
    }
    builder(sqe);
    pending_count_++;
    if (!next_is_timeout) {
        return io_uring_submit(&ring_);
    }
    return -EINPROGRESS;
}

UsefulCqe Ring::poll() const {
    struct io_uring_cqe* cqe;
    SYSCALL_VERIFY(io_uring_wait_cqe(&ring_, &cqe) == 0, "io_uring_wait_cqe");
    auto useful = UsefulCqe(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    pending_count_--;
    return useful;
}

std::optional<UsefulCqe> Ring::try_poll() const {
    struct io_uring_cqe* cqe;
    int ret = io_uring_peek_cqe(&ring_, &cqe);
    if (ret == -EAGAIN) {
        return std::nullopt;
    }
    SYSCALL_VERIFY(ret == 0, "io_uring_peek_cqe");
    auto useful = UsefulCqe(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    pending_count_--;
    return useful;
}

bool Ring::has_pending() const {
    return pending_count_ > 0;
}



}
