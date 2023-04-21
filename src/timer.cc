#include "timer.h"

Timer::Timer() {
    if (!clock_gettime(CLOCK_MONOTONIC, &start_)) {

    }
}
