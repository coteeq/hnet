#pragma once
#include "five/object_pool.h"
#include <sure/stack/mmap.hpp>
#include <variant>
#include <wheels/logging/logging.hpp>
#include <sure/context.hpp>
#include <netring/net.h>
#include <netring/uring.h>
#include <liburing.h>
#include <five/stdint.h>
#include <string_view>
#include <five/time.h>

namespace net {

std::string_view iov_as_text(struct iovec iov);

struct Request {
    Request(size_t iov_len) {
        iov.iov_base = malloc(iov.iov_len = iov_len);
        hdr.msg_iov = &iov;
        hdr.msg_iovlen = 1;
        hdr.msg_name = &addr;
        hdr.msg_namelen = sizeof(addr);
    }

    Request& operator=(const Request&) = delete;
    Request& operator=(Request&&) = delete;
    Request(const Request&) = delete;
    Request(Request&& other)
        : iov(other.iov)
        , hdr(other.hdr)
        , addr(other.addr)
        , cqe(other.cqe)
    {
        hdr.msg_iov = &iov;
        hdr.msg_name = &addr;
        for (size_t i = 0; i < hdr.msg_iovlen; ++i) {
            other.iov.iov_base = nullptr; // prevent free() of these buffers
        }
        other.clear();
    }

    std::string_view as_text() {
        return iov_as_text(iov);
    }

    void prepare_for_send(Addr to) {
        addr = to.addr6_;
    }

    struct iovec iov = {};
    struct msghdr hdr = {};
    struct sockaddr_in6 addr = {};
    UsefulCqe cqe;

    void clear() {
        // TODO: mem leak in operator=
        for (size_t i = 0; i < hdr.msg_iovlen; ++i) {
            free(hdr.msg_iov[i].iov_base);
            hdr.msg_iov[i].iov_len = 0;
        }
        hdr.msg_iov = nullptr;
        hdr.msg_name = nullptr;
        hdr.msg_namelen = 0;
        hdr.msg_iovlen = 0;
    }

    ~Request() {
        clear();
    }
};


class BaseHandler: public sure::ITrampoline {
public:
    virtual void handle(Uring* ring, Request* req) = 0;
    void Run() noexcept override;

    BaseHandler(BaseHandler&&) = default;

protected:
    BaseHandler(
        Uring* ring,
        Request&& request,
        sure::Stack&& stack,
        sure::ExecutionContext* main_ctx,
        Cookie cookie,
        Socket* socket,
        five::Instant start,
        five::ObjectPoolLite<Request>& requests_pool
    );

    void yield();

private:

    void switch_here();
    bool is_finished() const;
    sure::ExecutionContext* get_my_ctx();
    void setup();
    friend class Executor;

private:
    Uring* uring_ref_;
    Request request_;
    sure::Stack stack_;

    sure::ExecutionContext* main_ctx_;
    sure::ExecutionContext my_ctx_;
    bool finished_;

public:
    Socket* socket_;
    UsefulCqe last_cqe_;
    Cookie cookie_;
    five::ObjectPoolLite<Request>& requests_pool_;

private:
    five::Instant start_;
};

class Executor {
public:
    // using HandlerFactory = std::function<std::unique_ptr<BaseHandler>(sure::Stack&&)>;

    template <class Handler>
    void dispatch_loop(Socket* socket) {
        static_assert(std::is_base_of_v<BaseHandler, Handler>);
        sure::ExecutionContext main_ctx;
        using ReqOrHandler = std::variant<std::monostate, Request, Handler>;

        five::ObjectPoolLite<sure::Stack> stacks(10, [] { return sure::Stack::AllocateBytes(64 * 1024); });
        five::ObjectPoolLite<ReqOrHandler*> req_or_handler_pool(10, [] { return new ReqOrHandler; });
        five::ObjectPoolLite<Request> requests(10, [] { return Request(500); });

        {
            auto* req_or_handler = new ReqOrHandler(Request(500));
            uring_.recvmsg(socket, &std::get<Request>(*req_or_handler).hdr, static_cast<Cookie>(req_or_handler));
        }

        for (;;) {
            auto start = five::Instant::now();
            auto cqe = uring_.poll_cookie();
            LOG_INFO("polled in {}", five::Instant::now() - start);
            auto* req_or_handler = static_cast<ReqOrHandler*>(cqe.user_data);

            LOG_DEBUG("got req_or_handler: {}", fmt::ptr(req_or_handler));
            if (std::holds_alternative<Request>(*req_or_handler)) {
                auto&& req = std::get<Request>(std::move(*req_or_handler));
                auto* new_req_or_handler = req_or_handler_pool.get();
                auto h = Handler(
                    &uring_,
                    std::move(req),
                    std::move(stacks.get()),
                    &main_ctx,
                    static_cast<Cookie>(new_req_or_handler),
                    socket,
                    start,
                    requests
                );
                h.last_cqe_ = cqe;
                new_req_or_handler->template emplace<Handler>(std::move(h));
                auto& handler = std::get<Handler>(*new_req_or_handler);
                handler.setup();
                LOG_DEBUG("created new handler w stack {} and variant: {}", fmt::ptr(handler.stack_.MutView().Data()), fmt::ptr(new_req_or_handler));

                req_or_handler->template emplace<Request>(std::move(requests.get()));
                uring_.recvmsg(socket, &std::get<Request>(*req_or_handler).hdr, static_cast<Cookie>(req_or_handler));

                LOG_INFO("jumping into handler after {}", five::Instant::now() - start);
                handler.switch_here();
                if (handler.is_finished()) {
                    LOG_DEBUG("delete oneshot");
                    stacks.put(std::move(handler.stack_));
                    req_or_handler_pool.put(std::move(new_req_or_handler));
                }
            } else if (std::holds_alternative<Handler>(*req_or_handler)) {
                auto& handler = std::get<Handler>(*req_or_handler);
                handler.last_cqe_ = cqe;
                LOG_DEBUG("switching second time to {}", fmt::ptr(req_or_handler));
                LOG_DEBUG("stack: {}", fmt::ptr(handler.stack_.MutView().Data()));
                handler.switch_here();
                LOG_DEBUG("returned to main");
                if (handler.is_finished()) {
                    LOG_DEBUG("delete multishot");
                    stacks.put(std::move(handler.stack_));
                    req_or_handler_pool.put(std::move(req_or_handler));
                }
            } else {
                WHEELS_PANIC("unreachable" << __FILE__ << ':' << __LINE__);
            }
        }
    }

private:
    Uring uring_;
};

}
