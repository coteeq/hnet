#include <ctime>
#include <netring/net.h>
#include <netring/uring.h>
#include <netring/uf.h>
#include <stdio.h>
#include <five/config.h>
#include <five/program.h>
#include <fallible/error/make.hpp>
#include <fallible/error/throw.hpp>
#include <string_view>
#include <thread>
#include <wheels/logging/logging.hpp>
#include <time.h>
#include <fmt/format.h>

struct ServerConfig {
    int64_t port = 6666;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(port)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ServerConfig);


fallible::Error from_res(int res) {
    auto code = std::make_error_code(static_cast<std::errc>(-res));
    return fallible::Err(fallible::ErrorCodes::Internal)
        .Domain(code.category().name())
        .Reason(code.message())
        .Done();
}

class Handler: public net::BaseHandler {
public:
    using net::BaseHandler::BaseHandler;

    void handle(net::Uring* ring, net::Request* req) override {
        auto req_text = std::string_view(
            static_cast<const char*>(req->iov.iov_base),
            last_cqe_.res
        );

        // LOG_DEBUG("received req from={} text={} ({} bytes)",
        //     net::Addr::from_addr_in6(req->addr), req_text, req_text.size());

        // static const char* reply_text = "tenletters";
        net::Request reply(500);
        // LOG_DEBUG("iov_len: {}", reply.iov.iov_len);
        // auto fmt_res = fmt::format_to_n(
        //     static_cast<char*>(reply.iov.iov_base),
        //     reply.iov.iov_len,
        //     "in reply to '{}': {}",
        //     req_text, reply_text
        // );
        memcpy(reply.iov.iov_base, req->iov.iov_base, req_text.size());
        void* second_portion = static_cast<void*>(static_cast<char*>(reply.iov.iov_base) + req_text.size());
        memcpy(second_portion, req->iov.iov_base, req_text.size());
        reply.iov.iov_len = req_text.size() * 2;
        reply.addr = req->addr;

        // LOG_DEBUG("sending text to {}, {} bytes", net::Addr::from_addr_in6(req->addr), reply.as_text().size());

        // auto start = five::Instant::now();
        ring->sendmsg(socket_, &reply.hdr, cookie_);
        // LOG_INFO("submit: {}", start.elapsed());
        yield();

        // LOG_DEBUG("successfully sent {} bytes", last_cqe_.res)

        WHEELS_UNUSED(ring);
    }
};

class ServerProgram: public Program<ServerConfig> {
    int Run(const ServerConfig& config) override {
        auto socket = net::make_socket();
        socket->bind(net::Addr::ip6_any(config.port));

        net::Executor().dispatch_loop<Handler>(socket.get());

        return 0;
    }
};

int main(int argc, char **argv) {
    ServerProgram().SetupAndRun(argc, argv);
}
