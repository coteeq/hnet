#pragma once
#include <five/stdint.h>
#include <function2/function2.hpp>
#include <liburing.h>
#include <optional>


namespace net {

struct UsefulCqe {
    UsefulCqe() = default;
    UsefulCqe(const struct io_uring_cqe* cqe);

    void* user_data = 0;
    i32	  res = 0;
};

struct Starvation {
    size_t cycles = 0;
    size_t times = 0;
};

class Ring {
public:
    using RequestBuilder = fu2::function_view<void(struct io_uring_sqe*)>;

    Ring(u32 entries = 8, u32 cpuid = -1);

    int submit(RequestBuilder builder, bool next_is_timeout = false) const;
    UsefulCqe poll() const;
    std::optional<UsefulCqe> try_poll() const;
    int try_poll_many(UsefulCqe* dst) const;
    bool has_pending() const;

private:
    // TODO: buffers
    mutable struct io_uring ring_;
    mutable size_t pending_count_;
    mutable Starvation starvation_;
};

}
