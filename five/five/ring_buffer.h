#pragma once
#include <vector>

namespace five {

template <class T>
class RingBuffer {
public:
    RingBuffer(size_t max_size)
        : buffer_(max_size)
        , head_(0)
        , tail_(0)
    {}

    void push_back_unchecked(T val) {
        buffer_[head_++] = val;
    }

    T pop_unchecked() {
        return buffer_[tail_++];
    }

    size_t size() const {
        return (buffer_.size() + head_ - tail_) % buffer_.size();
    }


private:
    std::vector<T> buffer_;
    size_t head_;
    size_t tail_;
};

}
