#pragma once
#include <time.h>
#include <cstdint>

class Timer {
public:
    Timer();

    uint64_t elapsed_nanos() const;

private:
    struct timespec start_;
};
