#include "ring2/submitter.h"
#include "tf/rt/scheduler.hpp"
#include <ring2/addr.h>
#include <ring2/ring.h>
#include <ring2/poller.h>
#include <ring2/sock.h>
#include <five/config.h>
#include <five/program.h>
#include <five/time.h>
#include <fallible/error/make.hpp>
#include <wheels/logging/logging.hpp>
#include <numeric>

struct Address {
    std::string host;
    int64_t port;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(host)
        TS_VALUE(port)
    )
};
TOML_STRUCT_DEFINE_SAVELOAD(Address);

struct ClientConfig {
    std::string data_to_send = "ninebytes";
    int64_t port = 6667;
    Address server = Address{.host = "::1", .port = 6667};
    int64_t count = 3;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(data_to_send)
        TS_VALUE(server)
        TS_VALUE(port)
        TS_VALUE(count)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ClientConfig);

class ClientProgram: public Program<ClientConfig> {
    int Run(const ClientConfig& config) override {
        auto ring = std::make_shared<net::Ring>();
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);
        sched.Run([&config, submitter] {
            auto socket = net::UdpSocket();
            auto addr = net::Addr::from_parts(config.server.host, config.server.port);

            // const char* data_to_send = config.data_to_send.c_str();
            LOG_INFO("starting to send...");
            LOG_INFO("target: {}, socketfd: {}", addr, socket.fd());

            std::vector<five::Duration> timings;
            timings.reserve(config.count);

            auto start = five::Instant::now();
            for (int i = 0; i < config.count; ++i) {
                WHEELS_UNUSED(i);

                auto send_data = net::MsgHdr {
                    .buf = config.data_to_send,
                    .addr = addr,
                };
                LOG_INFO("sending...");
                auto send_res = submitter.sendmsg(socket.fd(), send_data);
                if (send_res <= 0) {
                    fallible::ThrowError(fallible::Err(fallible::FromErrno{-send_res}, WHEELS_HERE));
                }
                LOG_INFO("sent! res={}", send_res);

                auto result = submitter.recvmsg(socket.fd());

                if (result.buf == config.data_to_send) {
                    return;
                }

                auto end = five::Instant::now();
                timings.push_back(end - start);
                std::swap(start, end);
                // printf("received: '%s' (%zu bytes)\n", buffer, len2);
            }

            if (config.count > 1000) {
                timings = std::vector<five::Duration>(timings.begin() + 1000, timings.end());
            }

            std::sort(timings.begin(), timings.end());

            LOG_CRIT("min    = {}us", timings[0].micros());
            LOG_CRIT("max    = {}us", timings[timings.size() - 1].micros());
            LOG_CRIT(
                "avg    = {}us",
                std::accumulate(timings.begin(), timings.end(), five::Duration()).micros() / timings.size()
            );
            for (auto quantile : {50., 90., 95., 99., 99.9, 99.99}) {
                auto idx = size_t(timings.size() * (quantile / 100.));
                LOG_CRIT("p{:<5} = {}us", quantile, timings[idx].micros());
            }
        });

        return 0;
    }
};

int main(int argc, char **argv) {
    return ClientProgram().SetupAndRun(argc, argv);
}
