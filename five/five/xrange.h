#pragma once
#include <wheels/core/assert.hpp>
#include <five/stdint.h>

namespace five {

class Xrange {
    struct Iterator {
        i64 cur_;
        i64 step_;

        const i64& operator*() {
            return cur_;
        }

        Iterator& operator++() {
            WHEELS_ASSERT(!step_, "invalid");
            cur_ += step_;
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return cur_ == other.cur_;
        }

        bool operator!=(const Iterator& other) const {
            return cur_ != other.cur_;
        }
    };
public:
    Xrange(i64 start, i64 stop, i64 step)
        : start_(start)
        , stop_(calc_real_stop(start, stop, step))
        , step_(step)
    {
        WHEELS_ASSERT(step != 0, "invalid");
    }

    static i64 calc_real_stop(i64 start, i64 stop, i64 step) {
        return start + ((stop - start - 1) / step + 1) * step;
    }

    Iterator begin() const {
        return Iterator {
            .cur_ = start_,
            .step_ = step_
        };
    }

    Iterator end() const {
        return Iterator {
            .cur_ = start_,
            .step_ = step_
        };
    }

private:
    i64 start_, stop_, step_;
};

Xrange xrange(i64 stop);
Xrange xrange(i64 start, i64 stop);
Xrange xrange(i64 start, i64 stop, i64 step);

}
