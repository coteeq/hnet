#include "time.h"
#include <ctime>
#include <time.h>

i64 get_now_nanos(int clock_id) {
    struct timespec ts;
    if (clock_gettime(clock_id, &ts) != 0) {
        throw five::NowException(std::to_string(errno));
    }
    return ts.tv_nsec + ts.tv_sec * 1'000'000'000;
}

namespace five {

Instant Instant::now() {
    return Instant(get_now_nanos(CLOCK_REALTIME));
}

Instant Instant::now_fast() {
    return Instant(get_now_nanos(CLOCK_REALTIME_COARSE));
}

Duration Instant::operator -(const Instant& other) const {
    return Duration(nanos() - other.nanos());
}

Instant Instant::operator -(const Duration& other) const {
    return Instant(nanos() - other.nanos());
}

Instant Instant::operator +(const Duration& other) const {
    return Instant(nanos() + other.nanos());
}

Duration Duration::operator +(const Duration& other) const {
    return Duration(nanos() + other.nanos());
}
Duration Duration::operator -(const Duration& other) const {
    return Duration(nanos() - other.nanos());
}

std::ostream& operator <<(std::ostream& os, const five::Duration& dur) {
    i64 nanos = dur.nanos();
    const char* unit = "ns";
    i64 scale = 1;
    if (nanos > 2'000) { scale *= 1'000; unit = "us"; }
    if (nanos > 2'000'000) { scale *= 1'000; unit = "ms"; }
    if (nanos > 2'000'000'000) { scale *= 1'000; unit = "s"; }
    return os << nanos / scale << unit;
}
}
