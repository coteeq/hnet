#pragma once

#include <netring/net.h>
#include <netring/uring.h>

namespace net {

class Uco {



    SureCookie* request_holder_;
    sure::ExecutionContext* main_ctx_;
    sure::ExecutionContext my_ctx_;
    bool finished_;
    char stack_[1024 * 256];
    Uring* uring_;
};

}
