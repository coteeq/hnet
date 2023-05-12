#pragma once

#include "ring.h"
#include <tf/rt/poller.hpp>

namespace net {

class RingPoller: public tf::rt::IPoller {
public:
    RingPoller(std::shared_ptr<Ring> ring);

    tf::rt::Fiber* TryPoll() override;
    bool HasPending() const override;

private:
    std::shared_ptr<Ring> ring_;
};

struct UringResSetter {
    UringResSetter() = default;
    UringResSetter(int init_res);

    tf::rt::Fiber* fiber;
    int res;
    std::optional<std::function<bool()>> should_wake;
};

}
