#include "uf.h"
#include "wheels/logging/logging.hpp"
#include <wheels/memory/view_of.hpp>

namespace net {

std::string_view iov_as_text(struct iovec iov) {
    return {
        static_cast<const char*>(iov.iov_base),
        iov.iov_len
    };
}

BaseHandler::BaseHandler(
        Uring* ring,
        Request&& request,
        sure::Stack&& stack,
        sure::ExecutionContext* main_ctx,
        Cookie cookie,
        Socket* socket,
        five::Instant start,
        five::ObjectPoolLite<Request>& requests_pool
    )
    : uring_ref_(ring)
    , request_(std::move(request))
    , stack_(std::move(stack))
    , main_ctx_(main_ctx)
    , my_ctx_()
    , finished_(false)
    , socket_(socket)
    , last_cqe_()
    , cookie_(cookie)
    , requests_pool_(requests_pool)
    , start_(start)
{
}


void BaseHandler::Run() noexcept {
    LOG_INFO("calling handler after {}", five::Instant::now() - start_);
    try {
        LOG_DEBUG("handling request");
        handle(uring_ref_, &request_);
    } catch (std::exception& ex) {
        LOG_ERROR("handler died: {}", ex.what());
    }
    finished_ = true;

    LOG_DEBUG("ExitTo({})", fmt::ptr(main_ctx_));
    LOG_INFO("finished request in {}", five::Instant::now() - start_);
    my_ctx_.ExitTo(*main_ctx_);
}

void BaseHandler::yield() {
    LOG_DEBUG("get back to main...");
    get_my_ctx()->SwitchTo(*main_ctx_);
    LOG_DEBUG("inside fiber again");
}

bool BaseHandler::is_finished() const {
    return finished_;
}

sure::ExecutionContext* BaseHandler::get_my_ctx() {
    return &my_ctx_;
}

void BaseHandler::switch_here() {
    LOG_DEBUG("switching...");
    main_ctx_->SwitchTo(*get_my_ctx());
    LOG_DEBUG("switched!");
}

void BaseHandler::setup() {
    my_ctx_.Setup(stack_.MutView(), this);
}

// Executor::Executor(Executor::HandlerFactory&& factory)
//     : factory_(std::move(factory))
//     , uring_()
// {}

// void Executor::dispatch_loop(std::shared_ptr<Socket> socket) {
// }

}
