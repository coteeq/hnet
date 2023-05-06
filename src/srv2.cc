#include "wheels/logging/logging.hpp"
#include <five/config.h>
#include <five/program.h>
#include <ring2/ring.h>
#include <ring2/addr.h>
#include <ring2/poller.h>
#include <ring2/submitter.h>
#include <ring2/sock.h>
#include <tf/rt/scheduler.hpp>
#include <tf/sched/spawn.hpp>


struct ServerConfig {
    int64_t port = 6667;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(port)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ServerConfig);

const char REPLY[] = \
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 9\r\n"
    // "Connection: close\r\n"
    "\r\nninebytes";

class ServerProgram: public Program<ServerConfig> {
    int Run(const ServerConfig& config) override {
        auto ring = std::make_shared<net::Ring>();
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);

        sched.Run([&config, submitter] {
            auto socket = net::Socket(net::IPFamily::V4, net::Proto::TCP);
            socket.bind(net::Addr::ip4_any(config.port));
            socket.listen(10);

            for (;;) {
                auto [fd, addr] = submitter.accept(socket.fd());
                LOG_INFO("Got connection from {}", addr);
                tf::Spawn([&submitter, fd=fd] {
                    for (;;) {
                        auto req = submitter.recvmsg(fd);
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
        });

        return 0;
    }
};

int main(int argc, char **argv) {
    ServerProgram().SetupAndRun(argc, argv);
}
