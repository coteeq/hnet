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
    TimeBase(i64 nanos)
        : nanos_(nanos)
    {}

    i64 nanos() const {
        return nanos_;
    }
    i64 micros() const {
        return nanos_ / 1'000;
    }
    i64 seconds() const {
        return nanos_ / 1'000'000;
    }
    i64 millis() const {
        return nanos_ / 1'000'000'000;
    }
    double seconds_float() const {
        return (double)nanos_ / 1'000'000'000.0;
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

std::ostream& operator <<(std::ostream& os, const five::Duration& dur);
}

template <> struct fmt::formatter<five::Duration> : ostream_formatter {};
