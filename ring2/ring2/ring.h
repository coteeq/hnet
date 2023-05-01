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

class Ring {
public:
    using RequestBuilder = fu2::function_view<void(struct io_uring_sqe*)>;

    explicit Ring(u32 entries = 8);

    void submit(RequestBuilder builder) const;
    UsefulCqe poll() const;
    std::optional<UsefulCqe> try_poll() const;
    bool has_pending() const;

private:
    // TODO: buffers
    mutable struct io_uring ring_;
    mutable size_t pending_count_;
};

}
