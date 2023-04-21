#include <stdio.h>
#include <string.h>
#include <netring/net.h>
#include <fallible/error/make.hpp>
#include <fallible/error/throw.hpp>
#include <wheels/core/assert.hpp>
#include <five/config.h>
#include <wheels/core/assert.hpp>
#include <five/program.h>
#include <iostream>

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

    TOML_STRUCT_DEFINE_MEMBER_SAVELOAD(
        TS_VALUE(data_to_send)
        TS_VALUE(server)
        TS_VALUE(port)
    )
};

TOML_STRUCT_DEFINE_SAVELOAD(ClientConfig);

class ClientProgram: public Program<ClientConfig> {
    int Run(const ClientConfig& config) override {
        auto socket = net::make_socket();
        auto addr = net::Addr::from_parts(config.server.host, config.server.port);

        const char* data_to_send = config.data_to_send.c_str();
        fprintf(stderr, "sending...\n");

        ssize_t len = sendto(
            socket->fd(),
            data_to_send,
            strlen(data_to_send),
            0,
            &addr.addr_,
            addr.addrlen_
        );
        SYSCALL_VERIFY(len > 0, "sendto");

        fprintf(stderr, "sent! len=%ld\n", len);
        fprintf(stderr, "receiving...\n");

        char buffer[100];
        ssize_t len2 = recvfrom(socket->fd(), buffer, 99, 0, NULL, NULL);

        buffer[len2] = '\0';
        printf("received: '%s' (%zu bytes)\n", buffer, len2);

        return 0;
    }
};

int main(int argc, char **argv) {
    return ClientProgram().SetupAndRun(argc, argv);
}
