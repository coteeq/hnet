#pragma once
#include "stdint.h"
#include <iostream>
#include <stdexcept>
#include <fmt/ostream.h>

namespace five {


class NowException: public std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {

template <class T>
class TimeBase {
public:
    TimeBase() = default;
    explicit constexpr TimeBase(i64 nanos)
        : nanos_(nanos)
    {}
    explicit constexpr TimeBase(const struct timespec& ts)
        : nanos_((i64)ts.tv_sec * 1'000'000'000 + (i64)ts.tv_nsec)
    {}

    inline i64 nanos() const {
        return nanos_;
    }
    inline i64 micros() const {
        return nanos_ / 1'000;
    }
    inline i64 millis() const {
        return nanos_ / 1'000'000;
    }
    inline i64 seconds() const {
        return nanos_ / 1'000'000'000;
    }
    inline double seconds_float() const {
        return (double)nanos_ / 1'000'000'000.0;
    }

    struct timespec as_timespec() const {
        return {
            .tv_sec = nanos() / 1'000'000'000,
            .tv_nsec = nanos() % 1'000'000'000
        };
    }

    inline bool operator ==(const T& other) const {
        return nanos() == other.nanos();
    }
    inline bool operator <(const T& other) const {
        return nanos() < other.nanos();
    }
    inline bool operator <=(const T& other) const {
        return nanos() <= other.nanos();
    }
    inline bool operator !=(const T& other) const {
        return !(*this == other);
    }
    inline bool operator >(const T& other) const {
        return !(*this <= other);
    }
    inline bool operator >=(const T& other) const {
        return !(*this < other);
    }

private:
    i64 nanos_ = 0;
};

}

class Duration;
class Instant: public detail::TimeBase<Instant> {
public:
    using TimeBase::TimeBase;

    static Instant now();
    static Instant now_fast();

    Duration elapsed() const;
    Duration elapsed_fast() const;

    Duration operator -(const Instant& other) const;

    Instant operator +(const Duration& other) const;
    Instant operator -(const Duration& other) const;
};

class Duration: public detail::TimeBase<Duration> {
public:
    using TimeBase::TimeBase;

    Duration operator +(const Duration& other) const;
    Duration operator -(const Duration& other) const;
};

void linux_nanosleep(Duration dur);

std::ostream& operator <<(std::ostream& os, const five::Duration& dur);

namespace time_literals {

inline constexpr Duration operator""_ns(ull64 count) {
    return Duration(count);
}

inline constexpr Duration operator""_us(ull64 count) {
    return Duration(count * 1'000);
}

inline constexpr Duration operator""_ms(ull64 count) {
    return Duration(count * 1'000'000);
}

inline constexpr Duration operator""_s(ull64 count) {
    return Duration(count * 1'000'000'000);
}

}

}

template <> struct fmt::formatter<five::Duration> : ostream_formatter {};
