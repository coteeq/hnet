#pragma once

#include "config.h"
#include <wheels/core/assert.hpp>
#include <sstream>
#include <fmt/core.h>

template <class Config>
class Program {
public:
    int SetupAndRun(int argc, char **argv) {
        Config cfg;
        WHEELS_VERIFY(argc == 2, "argc should be 2");
        try {
            cfg.config_load(toml::parse_file(argv[1]));
            fmt::print(
                "starting with config:\n{}",
                [&cfg] {
                    std::ostringstream os;
                    print(os, cfg);
                    os << "\n";
                    return os.str();
                }());
            return Run(cfg);
        } catch (std::exception& ex) {
            std::cerr << "Die: " << ex.what() << std::endl;
            return 1;
        }
    }

    virtual int Run(const Config& config) = 0;
};
