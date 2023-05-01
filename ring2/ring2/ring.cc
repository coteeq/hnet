#include "ring.h"
#include "fallible/error/throw.hpp"

#include <fallible/error/make.hpp>
#include <wheels/logging/logging.hpp>


namespace net {

UsefulCqe::UsefulCqe(const struct io_uring_cqe* cqe)
    : user_data(io_uring_cqe_get_data(cqe))
    , res(cqe->res)
{}

Ring::Ring(u32 entries) {
    struct io_uring_params params = {};
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 2000;

    if (int res = io_uring_queue_init_params(entries, &ring_, &params); res != 0) {
        fallible::ThrowError(fallible::Err(fallible::FromErrno{-res}));
    }
    pending_count_ = 0;
}

void Ring::submit(RequestBuilder builder) const {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    builder(sqe);
    io_uring_submit(&ring_);
    pending_count_++;
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
