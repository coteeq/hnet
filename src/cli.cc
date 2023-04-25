#include "five/xrange.h"
#include "wheels/core/compiler.hpp"
#include <numeric>
#include <wheels/logging/logging.hpp>
#include <string.h>
#include <netring/net.h>
#include <fallible/error/make.hpp>
#include <fallible/error/throw.hpp>
#include <wheels/core/assert.hpp>
#include <five/config.h>
#include <wheels/core/assert.hpp>
#include <five/program.h>
#include <iostream>
#include <five/time.h>

struct Address {
    std::string host;
    int64_t port;

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(host)
        TS_VALUE(port)
    )
};
TOML_STRUCT_DEFINE_SAVELOAD(Address);

#define COMMA ,
struct ClientConfig {
    std::string data_to_send = "ninebytes";
    int64_t port = 6666;
    Address server = Address{.host = "::1", .port = 6666};
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
        auto socket = net::make_socket();
        auto addr = net::Addr::from_parts(config.server.host, config.server.port);

        const char* data_to_send = config.data_to_send.c_str();
        LOG_INFO("starting to send...");

        std::vector<five::Duration> timings;
        timings.reserve(config.count);

        auto start = five::Instant::now();
        for (int i = 0; i < config.count; ++i) {
            WHEELS_UNUSED(i);

            ssize_t len = sendto(
                socket->fd(),
                data_to_send,
                config.data_to_send.size(),
                0,
                &addr.addr_,
                addr.addrlen_
            );
            SYSCALL_VERIFY(len > 0, "sendto");

            // fprintf(stderr, "sent! len=%ld\n", len);
            // fprintf(stderr, "receiving...\n");

            char buffer[150];
            ssize_t len2 = recvfrom(socket->fd(), buffer, 99, 0, NULL, NULL);

            buffer[len2] = '\0';
            if (std::string_view(buffer, len2) == config.data_to_send) {
                return 0;
            }

            auto end = five::Instant::now();
            timings.push_back(end - start);
            std::swap(start, end);
            // printf("received: '%s' (%zu bytes)\n", buffer, len2);
        }

        std::sort(timings.begin(), timings.end());

        LOG_INFO("min    = {}us", timings[0].micros());
        LOG_INFO("max    = {}us", timings[timings.size() - 1].micros());
        LOG_INFO(
            "avg    = {}us",
            std::accumulate(timings.begin(), timings.end(), five::Duration()).micros() / timings.size()
        );
        for (auto quantile : {50., 90., 95., 99., 99.9, 99.99}) {
            auto idx = size_t(timings.size() * (quantile / 100.));
            LOG_DEBUG("p{:<5} = {}us", quantile, timings[idx].micros());
        }
        wheels::FlushPendingLogMessages();
        return 0;
    }
};

int main(int argc, char **argv) {
    return ClientProgram().SetupAndRun(argc, argv);
}
