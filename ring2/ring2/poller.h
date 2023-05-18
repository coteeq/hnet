#pragma once

#include "ring.h"
#include <tf/rt/poller.hpp>

namespace net {

class RingPoller: public tf::rt::IPoller {
public:
    RingPoller(std::shared_ptr<Ring> ring, size_t busypoll_attempts = 1'000'000);

    tf::rt::Fiber* TryPoll() override;
    int TryPollMany(tf::rt::Fiber*[tf::rt::MAX_POLL_MANY]) override;
    bool HasPending() const override;

private:
    std::shared_ptr<Ring> ring_;
    size_t attempts_;
    size_t max_attempts_;
};

struct UringResSetter {
    UringResSetter() = default;
    UringResSetter(int init_res);

    tf::rt::Fiber* fiber;
    int res;
    std::optional<std::function<bool()>> should_wake;
};

}
