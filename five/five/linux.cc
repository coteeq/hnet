#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

#include "linux.h"

namespace five {

int set_affinity(u64 mask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < sizeof(mask); ++i) {
        if (mask & (1ull << i)) {
            CPU_SET(i, &cpuset);
        }
    }

    return sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

}
