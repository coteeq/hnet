#include "wheels/core/assert.hpp"
#include "wheels/logging/logging.hpp"
#include "wheels/memory/view_of.hpp"
#include <deque>
#include <five/config.h>
#include <five/program.h>
#include <five/linux.h>
#include <ring2/ring.h>
#include <ring2/addr.h>
#include <ring2/poller.h>
#include <ring2/submitter.h>
#include <ring2/sock.h>
#include <tf/rt/scheduler.hpp>
#include <tf/sched/spawn.hpp>


struct ServerConfig {
    int64_t port = 6667;
    int64_t ring_entries = 8;
    std::string mode = "udp";
    int64_t ipv = 4;
    int64_t threads = 1;
    int64_t kernel_cpu = (u32)-1;
    int64_t app_aff = (u32)-1;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(port)
        TS_VALUE(ring_entries)
        TS_VALUE(mode)
        TS_VALUE(ipv)
        TS_VALUE(threads)
        TS_VALUE(kernel_cpu)
        TS_VALUE(app_aff)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ServerConfig);

const char REPLY[] = \
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 9\r\n"
    // "Connection: close\r\n"
    "\r\nninebytes";

class ServerProgram: public five::Program<ServerConfig> {
    int Run(const ServerConfig& config) override {
        auto ring = std::make_shared<net::Ring>(config.ring_entries, config.kernel_cpu);
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);

        if (auto ret = five::set_affinity(config.app_aff); ret != 0) {
            LOG_WARN("setaffinity: {}", ret);
        }

        sched.Run([&config, submitter] {
            auto sock_family = config.ipv == 4 ? net::IPFamily::V4 : net::IPFamily::V6;
            auto bind_addr = config.ipv == 4 ? net::Addr::ip4_any(config.port) : net::Addr::ip6_any(config.port);

            if (config.mode == "tcp") {
                auto socket = net::Socket(sock_family, net::Proto::TCP);
                socket.bind(bind_addr);
                socket.listen(10);

                for (;;) {
                    auto [fd, addr] = submitter.accept(socket.fd());
                    LOG_INFO("Got connection from {}", addr);
                    tf::Spawn([&submitter, fd=fd, addr=addr] {

                        for (;;) {
                            auto req = submitter.recvmsg(fd);
                            if (req.raw_ret == -ECONNRESET) {
                                LOG_ERROR("Conn reset for addr {}", addr);
                                break;
                            }
                            if (req.buf.size() == 0) {
                                break;
                            }
                            LOG_INFO("Received req: from: {}, text: {} raw_ret = {}", req.addr, req.buf, req.raw_ret);
                            net::MsgHdr hdr = {
                                .buf = REPLY,
                                .addr = req.addr,
                            };
                            submitter.sendmsg(fd, hdr);
                        }
                    }).Detach();
                }
            } else if (config.mode == "udp") {
                auto socket = net::Socket(sock_family, net::Proto::UDP);
                socket.bind(bind_addr);
                std::deque<tf::JoinHandle> fibers;

                for (int i = 0; i < config.threads; ++i) {
                    LOG_INFO("in for {}", i);
                    fibers.emplace_back(tf::Spawn([i, &submitter, &socket] {
                        LOG_INFO("inside {}", i);
                        char buf[65'000];

                        for (;;) {
                            auto msg = submitter.recvmsg(socket.fd(), wheels::MutViewOf(buf));
                            LOG_INFO("recv msg {} bytes", msg.buf.Size());
                            WHEELS_VERIFY(msg.buf.Size() * 2 < sizeof(buf), "Too long message");
                            {
                                auto* second = buf + msg.buf.Size();
                                memcpy(second, buf, msg.buf.Size());
                            }
                            auto ww = net::WriteView {
                                .buf = wheels::ConstMemView{buf, msg.buf.Size() * 2},
                                .addr = msg.addr
                            };
                            submitter.sendmsg(socket.fd(), ww);
                        }
                    }));
                }

                for (auto& jh : fibers) {
                    jh.Join();
                }
            }
        });

        return 0;
    }
};

int main(int argc, char **argv) {
    ServerProgram().SetupAndRun(argc, argv);
}
