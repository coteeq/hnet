#include "ring.h"

#include <fallible/error/make.hpp>


namespace net {

UsefulCqe::UsefulCqe(const struct io_uring_cqe* cqe)
    : user_data(io_uring_cqe_get_data(cqe))
    , res(cqe->res)
{}

Ring::Ring(u32 entries) {
    struct io_uring_params params = {};
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 2000;

    io_uring_queue_init_params(entries, &ring_, &params);
}

void Ring::submit(RequestBuilder builder) const {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    builder(sqe);
    io_uring_submit(&ring_);
}

UsefulCqe Ring::poll() const {
    struct io_uring_cqe* cqe;
    SYSCALL_VERIFY(io_uring_wait_cqe(&ring_, &cqe) == 0, "io_uring_wait_cqe");
    auto useful = UsefulCqe(cqe);
    io_uring_cqe_seen(&ring_, cqe);
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
    return useful;
}



}
