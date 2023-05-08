#include "ring2/submitter.h"
#include "tf/rt/scheduler.hpp"
#include "tf/sched/spawn.hpp"
#include "tf/sync/join_handle.hpp"
#include "wheels/memory/view_of.hpp"
#include <deque>
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
    Address server = Address{.host = "127.0.0.1", .port = 6667};
    int64_t count = 3;
    int64_t threads = 1;
    int64_t ring_entries = 8;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(data_to_send)
        TS_VALUE(server)
        TS_VALUE(port)
        TS_VALUE(count)
        TS_VALUE(threads)
        TS_VALUE(ring_entries)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ClientConfig);

class ClientProgram: public five::Program<ClientConfig> {
    int Run(const ClientConfig& config) override {
        auto ring = std::make_shared<net::Ring>(config.ring_entries);
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);

        auto data_to_send = config.data_to_send;
        for (int i = 0; i < 10; ++i) {
            data_to_send = data_to_send + data_to_send;
        }
        LOG_CRIT("one msg: {}", data_to_send.size());

        sched.Run([&config, submitter, &data_to_send] {
            auto socket = net::Socket(net::IPFamily::V4, net::Proto::UDP);
            auto addr = net::Addr::from_parts(config.server.host, config.server.port, net::IPFamily::V4);

            // const char* data_to_send = config.data_to_send.c_str();
            LOG_INFO("starting to send...");
            LOG_INFO("target: {}, socketfd: {}", addr, socket.fd());

            std::vector<five::Duration> timings;
            timings.reserve(config.count * config.threads);

            std::deque<tf::JoinHandle> fibers;

            for (int thr = 0; thr < config.threads; ++thr) {
                fibers.emplace_back(tf::Spawn([config, addr, submitter, &socket, &timings, thr, &data_to_send] {
                    auto start = five::Instant::now();
                    LOG_CRIT("in thread {}", thr);
                    for (int i = 0; i < config.count; ++i) {
                        WHEELS_UNUSED(i);

                        auto send_data = net::MsgHdr {
                            .buf = data_to_send,
                            .addr = addr,
                        };
                        LOG_DEBUG("sending...");
                        auto send_res = submitter.sendmsg(socket.fd(), send_data);
                        if (send_res <= 0) {
                            fallible::ThrowError(fallible::Err(fallible::FromErrno{-send_res}, WHEELS_HERE));
                        }

                        char buf[65'000];
                        auto result = submitter.recvmsg(socket.fd(), wheels::MutViewOf(buf));
                        LOG_DEBUG("got reply!");

                        if (wheels::ConstMemView(result.buf).AsStringView() == config.data_to_send) {
                            return;
                        }

                        auto end = five::Instant::now();
                        timings.push_back(end - start);
                        std::swap(start, end);
                    }
                }));
            }

            for (auto& jh : fibers) {
                jh.Join();
            }

            if (timings.size() > 1000) {
                LOG_WARN("Cutting first 1000 records");
                timings = std::vector<five::Duration>(timings.begin() + 1000, timings.end());
            }

            LOG_CRIT("Total: {} requests", timings.size());

            std::sort(timings.begin(), timings.end());

            LOG_CRIT("min    = {}", timings[0]);
            LOG_CRIT("max    = {}", timings[timings.size() - 1]);
            LOG_CRIT(
                "avg    = {}us",
                std::accumulate(timings.begin(), timings.end(), five::Duration()).micros() / timings.size()
            );
            for (auto quantile : {50., 90., 95., 99., 99.9, 99.99}) {
                auto idx = size_t(timings.size() * (quantile / 100.));
                LOG_CRIT("p{:<5} = {}", quantile, timings[idx]);
            }

            auto traffic_out = static_cast<double>(timings.size() * data_to_send.size()) / 1'000'000;
            LOG_CRIT("throughput: out: {}mb/s, in: {}mb/s", traffic_out, traffic_out * 2);
        });

        return 0;
    }
};

int main(int argc, char **argv) {
    return ClientProgram().SetupAndRun(argc, argv);
}
