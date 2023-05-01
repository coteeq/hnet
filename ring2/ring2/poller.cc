#include "poller.h"
#include <tf/rt/fiber.hpp>
#include <wheels/logging/logging.hpp>

namespace net {

RingPoller::RingPoller(std::shared_ptr<Ring> ring)
    : ring_(std::move(ring))
{}

tf::rt::Fiber* RingPoller::TryPoll() {
    auto maybe_cqe = ring_->try_poll();
    if (maybe_cqe) {
        auto* setter = static_cast<UringResSetter*>(maybe_cqe->user_data);
        setter->res = maybe_cqe->res;
        return setter->fiber;
    }
    return nullptr;
}

bool RingPoller::HasPending() const {
    return ring_->has_pending();
}

}
