#include <five/config.h>
#include <five/program.h>
#include <ring2/ring.h>
#include <ring2/addr.h>
#include <ring2/poller.h>
#include <ring2/submitter.h>
#include <ring2/sock.h>
#include <tf/rt/scheduler.hpp>


struct ServerConfig {
    int64_t port = 6667;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(port)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ServerConfig);


class ServerProgram: public Program<ServerConfig> {
    int Run(const ServerConfig& config) override {
        auto ring = std::make_shared<net::Ring>();
        auto poller = net::RingPoller(ring);
        auto sched = tf::rt::Scheduler(&poller);
        auto submitter = net::Submitter(ring);

        sched.Run([&config, submitter] {
            auto socket = net::Socket(net::IPFamily::V4, net::Proto::UDP);
            socket.bind(net::Addr::ip4_any(config.port));

            for (;;) {
                auto msg = submitter.recvmsg(socket.fd());
                LOG_INFO("recv msg {} bytes", msg.buf.size());
                msg.buf = msg.buf + msg.buf;
                submitter.sendmsg(socket.fd(), msg);
            }
        });

        return 0;
    }
};

int main(int argc, char **argv) {
    ServerProgram().SetupAndRun(argc, argv);
}
