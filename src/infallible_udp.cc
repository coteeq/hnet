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
#include <five/linux.h>
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
    int64_t port = 6668;
    Address server = Address{.host = "127.0.0.1", .port = 6667};
    int64_t count = 3;
    int64_t threads = 1;
    int64_t ring_entries = 8;
    int64_t kernel_cpu = (u32)-1;
    int64_t app_aff = (u32)-1;
    int64_t data_len = 1;
    int64_t dead_packet_us = 1000; // 1ms


    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(data_to_send)
        TS_VALUE(server)
        TS_VALUE(port)
        TS_VALUE(count)
        TS_VALUE(threads)
        TS_VALUE(ring_entries)
        TS_VALUE(kernel_cpu)
        TS_VALUE(app_aff)
        TS_VALUE(data_len)
        TS_VALUE(dead_packet_us)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ClientConfig);

using namespace five::time_literals;

class ClientProgram: public five::Program<ClientConfig> {
    int Run(const ClientConfig& config) override {
        auto ring = std::make_shared<net::Ring>(config.ring_entries, config.kernel_cpu);
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);

        if (auto ret = five::set_affinity(config.app_aff); ret != 0) {
            LOG_WARN("setaffinity: {}", ret);
        }

        auto data_to_send = config.data_to_send;
        while (data_to_send.size() < (size_t)config.data_len) {
            data_to_send = data_to_send + config.data_to_send;
        }
        LOG_CRIT("one msg: {}", data_to_send.size());

        sched.Run([&config, submitter, &data_to_send] {

            auto addr = net::Addr::from_parts(config.server.host, config.server.port, net::IPFamily::V4);

            // const char* data_to_send = config.data_to_send.c_str();
            LOG_INFO("starting to send...");
            // LOG_INFO("target: {}, socketfd: {}", addr, socket.fd());

            std::vector<five::Duration> timings;
            timings.reserve(config.count * config.threads);

            std::deque<tf::JoinHandle> fibers;
            int dead_packets = 0;

            auto the_start = five::Instant::now();
            auto first_stop = five::Duration();
            auto first_stop_req_count = 0;

            for (int thr = 0; thr < config.threads; ++thr) {
                fibers.emplace_back(tf::Spawn([config, addr, submitter, &timings, thr, data_to_send, &dead_packets, &the_start, &first_stop, &first_stop_req_count] {
                    LOG_CRIT("in thread {}", thr);
                    auto socket = net::Socket(net::IPFamily::V4, net::Proto::UDP);
                    LOG_INFO("Binding to localhost:{}", config.port + thr);
                    socket.bind(net::Addr::ip4_any(config.port + thr));
                    auto data_expected = data_to_send + data_to_send;
                    LOG_INFO("connecting to", thr);

                    auto start = five::Instant::now();
                    char buf[65'000];
                    for (int i = 0; i < config.count / config.threads; ++i) {
                        WHEELS_UNUSED(i);

                        auto send_data = net::MsgHdr {
                            .buf = data_to_send,
                            .addr = addr,
                        };
                        LOG_DEBUG("sending...");
                        auto send_res = submitter.sendmsg(socket.fd(), send_data);
                        if (send_res <= 0) {
                            fallible::ThrowError(fallible::Err(fallible::FromErrno{-send_res}, "bad sendmsg", WHEELS_HERE));
                        }
                        LOG_DEBUG("sent!");

                        auto result = submitter.recvmsg(socket.fd(), wheels::MutViewOf(buf), five::Duration(config.dead_packet_us * 1000));
                        if (result.ret == -net::ESQE_TIMEOUT || result.ret == -EINTR) {
                            if (result.ret == -EINTR) {
                                LOG_WARN("dead EINTR packet");
                            }
                            ++dead_packets;
                            start = five::Instant::now();
                            continue;
                        }
                        if (result.ret <= 0) {
                            fallible::ThrowError(fallible::Err(fallible::FromErrno{-result.ret}, "bad recvmsg", WHEELS_HERE));
                        }
                        LOG_DEBUG("got reply!");

                        if (wheels::ConstMemView(result.buf).AsStringView() != data_expected) {
                            WHEELS_PANIC("bad response");
                        }

                        auto end = five::Instant::now();
                        timings.push_back(end - start);
                        std::swap(start, end);
                    }

                    if (first_stop.nanos() == 0) {
                        first_stop = the_start.elapsed();
                        first_stop_req_count = timings.size();
                    }
                }));
            }

            for (auto& jh : fibers) {
                jh.Join();
            }

            // auto wall_time = std::accumulate(wall_times.begin(), wall_times.end(), 0.0);

            if (timings.size() > 1000) {
                LOG_WARN("Cutting first 1000 records");
                timings = std::vector<five::Duration>(timings.begin() + 1000, timings.end());
            }

            LOG_CRIT("Total: {} requests", timings.size());
            LOG_CRIT("Total: {} dead packets", dead_packets);

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

            auto traffic_out_total = static_cast<double>(first_stop_req_count * data_to_send.size()) / 1'000'000;
            auto traffic_out = traffic_out_total / first_stop.seconds_float();
            LOG_CRIT("throughput: out: {}mb/s, in: {}mb/s", traffic_out, traffic_out * 2);
            LOG_CRIT("pps: {}", first_stop_req_count / first_stop.seconds_float());
        });

        LOG_CRIT("stopped");

        return 0;
    }
};

int main(int argc, char **argv) {
    return ClientProgram().SetupAndRun(argc, argv);
}
