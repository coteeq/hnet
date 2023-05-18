#pragma once

#include "config.h"
#include <wheels/core/assert.hpp>
#include <wheels/logging/logging.hpp>
#include <sstream>
#include <fmt/core.h>


namespace five {

template <class Config>
class Program {
public:
    int SetupAndRun(int argc, char **argv) {
        Config cfg;
        WHEELS_VERIFY(argc == 2, "argc should be 2");
        try {
            cfg.config_load(toml::parse_file(argv[1]));
            LOG_CRIT(
                "starting with config:\n{}",
                [&cfg] {
                    std::ostringstream os;
                    print(os, cfg);
                    os << "\n";
                    return os.str();
                }());
            auto res = Run(cfg);
            wheels::FlushPendingLogMessages();
            return res;
        } catch (std::exception& ex) {
            wheels::FlushPendingLogMessages();
            std::cerr << "Die: " << ex.what() << std::endl;
            return 1;
        }
    }

    virtual int Run(const Config& config) = 0;
};

}
