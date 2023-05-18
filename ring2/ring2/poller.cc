#include "poller.h"
#include "tf/rt/scheduler.hpp"
#include <tf/rt/fiber.hpp>
#include <wheels/logging/logging.hpp>

namespace net {


__attribute__((noinline))
tf::rt::Fiber* dispatch(const UsefulCqe& cqe) {
    auto* setter = static_cast<UringResSetter*>(cqe.user_data);
    setter->res = cqe.res;
    if (!setter->should_wake || (*setter->should_wake)()) {
        return setter->fiber;
    }

    return nullptr;
}


RingPoller::RingPoller(std::shared_ptr<Ring> ring, size_t busypoll_attempts)
    : ring_(std::move(ring))
    , attempts_(0)
    , max_attempts_(busypoll_attempts)
{}

tf::rt::Fiber* RingPoller::TryPoll() {
    auto maybe_cqe = ring_->try_poll();
    attempts_++;
    if (attempts_ >= max_attempts_) {
        maybe_cqe = ring_->poll();
    }
    if (maybe_cqe) {
        attempts_ = 0;
        return dispatch(*maybe_cqe);
    }
    return nullptr;
}

int RingPoller::TryPollMany(tf::rt::Fiber* fibers[tf::rt::MAX_POLL_MANY]) {
    UsefulCqe usefuls[32];
    int ret = ring_->try_poll_many(usefuls);
    for (int i = 0; i < ret; ++i) {
        fibers[i] = dispatch(usefuls[i]);
    }
    return ret;
}

bool RingPoller::HasPending() const {
    return ring_->has_pending();
}

UringResSetter::UringResSetter(int init_res)
    : fiber(tf::rt::Scheduler::Current()->RunningFiber())
    , res(init_res)
    , should_wake(std::nullopt)
{}

}
