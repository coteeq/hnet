#pragma once

#include "ring.h"
#include <tf/rt/poller.hpp>

namespace net {

class RingPoller: public tf::rt::IPoller {
public:
    RingPoller(std::shared_ptr<Ring> ring);

    tf::rt::Fiber* TryPoll() override;

private:
    std::shared_ptr<Ring> ring_;
};

struct UringResSetter {
    tf::rt::Fiber* fiber;
    int res;
};

}