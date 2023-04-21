#pragma once

#include <functional>
#include <sure/context.hpp>
#include <netinet/in.h>
#include <memory>
#include "five/object_pool.h"
#include "uring.h"
#include "net.h"
#include <wheels/intrusive/list.hpp>
#include <wheels/memory/view_of.hpp>
#include <variant>
#include <iostream>

namespace net {

template<class> inline constexpr bool always_false_v = false;



struct Request {
    Request(size_t iov_len) {
        iov.iov_base = malloc(iov.iov_len = iov_len);
        hdr.msg_iov = &iov;
        hdr.msg_iovlen = 1;
        hdr.msg_name = &addr;
        hdr.msg_namelen = sizeof(addr);
    }

    void prepare_for_send(Addr to) {
        addr = to.addr6_;
    }

    struct iovec iov = {};
    struct msghdr hdr = {};
    struct sockaddr_in6 addr = {};
    struct io_uring_cqe cqe = {};

    ~Request() {
        for (size_t i = 0; i < hdr.msg_iovlen; ++i) {
            free(hdr.msg_iov[i].iov_base);
        }
    }
};


class BaseHandler: sure::ITrampoline {
public:
    virtual void handle(Uring* ring, Request* req) = 0;
    void Run() noexcept override {
        try {
            handle(uring_ref_, request_.get());
        } catch (std::exception& ex) {
            std::cerr << "handler died: " << ex.what() << std::endl;
        }
        my_ctx_.ExitTo(*main_ctx_);
    }

    void setup(
        sure::ExecutionContext* main_ctx,
        Uring* uring,
        Request&& req
    ) {
        request_ = std::make_unique<Request>(std::move(req));
        uring_ref_ = uring;
        main_ctx_ = main_ctx;
        my_ctx_.Setup(wheels::MutViewOf(stack_), this);
        finished_ = false;
    }

    bool is_finished() const {
        return finished_;
    }

    sure::ExecutionContext* get_my_ctx() {
        return &my_ctx_;
    }

    void switch_here() {
        main_ctx_->SwitchTo(*get_my_ctx());
    }

private:
    BaseHandler(sure::ExecutionContext* main_ctx);
    template <class T>
    friend void dispatch_forever(std::shared_ptr<Socket> socker, Uring& uring);

private:
    Uring* uring_ref_;
    std::unique_ptr<Request> request_;
    sure::ExecutionContext* main_ctx_;
    sure::ExecutionContext my_ctx_;
    bool finished_;
    char stack_[1024 * 256];
};


template <class Handler>
struct SureCookie : wheels::IntrusiveListNode<SureCookie<Handler>> {

    void* as_raw() {
        return reinterpret_cast<void*>(this);
    }

    static SureCookie* from_raw(void* raw) {
        return reinterpret_cast<SureCookie*>(raw);
    }

    void reset() {
        content = std::monostate{};
    }

    Handler& as_handler() {
        return std::get<Handler>(content);
    }

    sure::ExecutionContext& as_ctx() {
        return std::get<sure::ExecutionContext>(content);
    }

    std::variant<std::monostate, Handler, Request> content;
};

// using CookiePtr = five::EitherPtr2<sure::ExecutionContext, Request>;



template <class Handler>
void dispatch_forever(std::shared_ptr<Socket> socket, Uring* uring) {
    sure::ExecutionContext main_ctx;
    five::ObjectPool<SureCookie<Handler>> cookies_pool(2);
    five::ObjectPool<Handler> handlers_pool(2);

    {
        auto* cookie = cookies_pool.Get();
        cookie->content = Request(500);
        uring->recvmsg(socket, &cookie->as_req().hdr, cookie->as_raw());
    }
    for (;;) {
        auto event = uring->poll_cookie();
        auto* cookie = SureCookie<Handler>::from_raw(event.first);
        std::visit(
            [&](auto&& var) {
                if constexpr (std::is_same_v<std::decay_t<decltype(var)>, Handler>) {
                    main_ctx.SwitchTo(var.get_my_ctx());
                    if (var.is_finished()) {
                        cookies_pool.Put(cookie);
                    }
                } else if constexpr (std::is_same_v<std::decay_t<decltype(var)>, Request>) {
                    auto* cookie = cookies_pool.Get();
                    auto& handler = cookie->as_handler();
                    handler.setup(&main_ctx, uring, std::move(var));
                    handler.switch_here();
                    if (handler.is_finished()) {
                        cookies_pool.Put(cookie);
                    }
                } else {
                    static_assert(always_false_v<decltype(var)>, "non-exhaustive visitor!");
                }
            },
            cookie->content
        );
    }
}


}


/*

static thread_local UFiber* current;

struct UFiber {


    Run() {
        try {
            handle()
        }
    }

    cqe last_cqe;
    uring ring;
    self_ctx;
    main_ctx;
};

sock = uring->accept();
handle = spawn(do_shit, uring, sock);

do_shit(the_context, sock) {
    the_context->read();
        uring->submit(read, hdr, ...)
        switch(main);
        res = the_context.res;

    uring->send();
        uring->submit()
        switch(main)
        res = get_local_cqe().res

    ExitTo(main)
}

struct Acceptor {
    void accept(int sock) = 0;
};




*/
