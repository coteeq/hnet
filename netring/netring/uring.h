#pragma once

#include "net.h"
#include <liburing.h>
#include <deque>
#include <functional>
#include <five/stdint.h>

namespace net {

using CompleteCb = std::function<void(struct io_uring_cqe*)>;
using Cookie = void*;


struct UsefulCqe {
    UsefulCqe() = default;
    UsefulCqe(const struct io_uring_cqe* cqe);

    void* user_data = 0;
    i32	  res = 0;
};


class Uring {
public:
    Uring();
    ~Uring();
    struct io_uring* get_ring();

    void sendmsg(std::shared_ptr<Socket> sock, struct msghdr* hdr, CompleteCb&& cb);
    void sendmsg(Socket* sock, struct msghdr* hdr, Cookie);
    void recvmsg(std::shared_ptr<Socket> sock, struct msghdr* hdr, CompleteCb&& cb);
    void recvmsg(Socket* sock, struct msghdr* hdr, Cookie, bool nosubmit = false);
    void poll();
    UsefulCqe poll_cookie();

private:
    struct UContext;

private:
    struct io_uring ring_;
    std::deque<UContext*> contexts_pool_;
};

}
