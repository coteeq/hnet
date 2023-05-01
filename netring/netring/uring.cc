#include "uring.h"

#include <fallible/error/make.hpp>
#include <fallible/error/throw.hpp>
#include <wheels/core/compiler.hpp>
#include <wheels/logging/logging.hpp>
#include <five/time.h>

#include <sched.h>

#include "liburing.h"
#include "liburing/io_uring.h"

using namespace five::time_literals;

namespace net {

struct Uring::UContext {
    CompleteCb cb;
};

UsefulCqe::UsefulCqe(const struct io_uring_cqe* cqe)
    : user_data(io_uring_cqe_get_data(cqe))
    , res(cqe->res)
{}

Uring::Uring()
    : ring_({})
    , contexts_pool_()
{
    struct io_uring_params params = {};
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 2000;

    io_uring_queue_init_params(8, &ring_, &params);
    for (int i = 0; i < 1000; ++i) {
        contexts_pool_.push_back(new UContext);
    }
}

Uring::~Uring() {
    for (auto* ctx : contexts_pool_) {
        delete ctx;
    }
    io_uring_queue_exit(&ring_);
}

void Uring::sendmsg(std::shared_ptr<Socket> sock, struct msghdr *hdr, CompleteCb&& cb) {
    auto* ctx = contexts_pool_.front();
    contexts_pool_.pop_front();
    ctx->cb = std::move(cb);

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_sendmsg(sqe, sock->fd(), hdr, 0);
    io_uring_sqe_set_data(sqe, ctx);
    io_uring_submit(&ring_);
}

void Uring::sendmsg(Socket* sock, struct msghdr *hdr, Cookie cookie) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_sendmsg(sqe, sock->fd(), hdr, 0);
    io_uring_sqe_set_data(sqe, cookie);
    io_uring_submit(&ring_);
}

void Uring::recvmsg(std::shared_ptr<Socket> sock, struct msghdr *hdr, CompleteCb &&cb) {
    auto* ctx = contexts_pool_.front();
    contexts_pool_.pop_front();
    ctx->cb = std::move(cb);

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_recvmsg(sqe, sock->fd(), hdr, 0);
    io_uring_sqe_set_data(sqe, ctx);
    io_uring_submit(&ring_);
}

void Uring::recvmsg(Socket* sock, struct msghdr *hdr, Cookie cookie, bool nosubmit) {
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_recvmsg(sqe, sock->fd(), hdr, 0);
    io_uring_sqe_set_data(sqe, cookie);
    if (!nosubmit) {
        io_uring_submit(&ring_);
    }
}

void Uring::poll() {
    struct io_uring_cqe* cqe;
    SYSCALL_VERIFY(io_uring_wait_cqe(&ring_, &cqe) == 0, "io_uring_wait_cqe");
    auto* ctx = static_cast<UContext*>(io_uring_cqe_get_data(cqe));
    ctx->cb(cqe);
    ctx->cb = {};
    contexts_pool_.push_back(ctx);
    io_uring_cqe_seen(&ring_, cqe);
}

UsefulCqe Uring::poll_cookie() {
    struct io_uring_cqe* cqe = nullptr;
    int ret = -1;
    for (int i = 0; i < 150000000; ++i) {
        ret = io_uring_peek_cqe(&ring_, &cqe);
        // LOG_INFO("peek: {}", ret);
        if (ret == 0) {
            break;
        }
        // five::linux_nanosleep(5_us);
    }
    if (ret != 0) {
        LOG_WARN("falling back to kernel wait");
        // auto start = five::Instant::now();
        SYSCALL_VERIFY(io_uring_wait_cqe(&ring_, &cqe) == 0, "io_uring_wait_cqe");
        // LOG_INFO("wait: {}", start.elapsed());
    }
    auto useful = UsefulCqe(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    return useful;
}

}
