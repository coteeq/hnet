#pragma once
#include "wheels/core/assert.hpp"
#include <new>
#include <utility>
#include <cstdint>
#include <memory>
#include <iostream>

namespace five {

template <typename T, int n_bits>
class BitPointer {
    static_assert(n_bits >= 1);

public:
    template <typename T_inner, int n_bits_inner, typename... Args>
    static BitPointer make(Args&& ... args) {
        BitPointer<T_inner, n_bits_inner> bp;
        bp.t_ = new (std::align_val_t(bp.alignment())) T(std::forward(args...));
        WHEELS_ASSERT(bp.t % bp.alignment() == 0, "invalid alignment");
        return bp;
    }

    BitPointer(T* ptr)
        : t_(ptr)
    {}

    ~BitPointer() {
        delete t_;
    }

    constexpr uintptr_t mask() const {
        return (1ull << n_bits) - 1;
    }

    constexpr int alignment() const {
        return 1 << (n_bits);
    }

    int get_bits() const {
        return static_cast<uintptr_t>(t_) & mask();
    }

    int set_bits(int bits) {
        bits = bits & mask();
        t_ &= ~static_cast<uintptr_t>(mask());
        t_ |= bits;
    }

    T* get() {
        return t_ & ~mask();
    }

    void* get_raw() {
        return t_;
    }

    BitPointer(const BitPointer&) = delete;
    BitPointer& operator =(const BitPointer&) = delete;
private:
    BitPointer() = default;
    T* t_;
};


template <class T1, class T2>
class EitherPtr {
public:
    template <class... Args>
    static EitherPtr make_first(Args&&... args) {
        return EitherPtr {
            .p1 = BitPointer<T1, 1>::make(std::forward(args...))
        };
    }

    template <class... Args>
    static EitherPtr make_second(Args&&... args) {
        return EitherPtr {
            .p2 = BitPointer<T2, 1>::make(std::forward(args...))
        };
    }

    static EitherPtr from_raw(void* ptr) {
        return EitherPtr {
            .p1 = BitPointer<T1, 1>(ptr)
        };
    }

    bool is_first() const {
        return !is_second();
    }

    bool is_second() const {
        return p1.get_bits();
    }

    T1* get_first() {
        WHEELS_VERIFY(is_first(), "");
        return p1.get();
    }

    T2* get_second() {
        WHEELS_VERIFY(is_second(), "");
        return p2.get();
    }

    void* get_raw() {
        return p1.get_raw();
    }

    ~EitherPtr() {
        if (is_first()) {
            p1->~BitPointer();
        } else {
            p2->~BitPointer();
        }
    }

private:
    union {
        BitPointer<T1, 1> p1;
        BitPointer<T2, 1> p2;
    };
};


namespace {
    template <int n>
    constexpr unsigned log2() {
        static_assert(n >= 1 && n <= 32);
        if (n <= 1)  { return 0; }
        if (n <= 2)  { return 1; }
        if (n <= 4)  { return 2; }
        if (n <= 8)  { return 3; }
        if (n <= 16) { return 4; }
        if (n <= 32) { return 5; }
        return 0; // unreachable
    }

    template<int N, typename... Ts> using nth_type_of =
        typename std::tuple_element<N, std::tuple<Ts...>>::type;

}



template <class... Args>
class EitherPtr2 {
public:
    static constexpr uintptr_t mask() {
        return static_cast<uintptr_t>(1ull << bit_count()) - 1ull;
    }

    static constexpr int bit_count() {
        return log2<sizeof...(Args)>();
    }

    static constexpr int alignment() {
        return 1 << bit_count();
    }

    void* get_raw() {
        return ptr_;
    }

    template <int N>
    const nth_type_of<N, Args...>* get() const {
        return reinterpret_cast<const nth_type_of<N, Args...>*>(get_aligned());
    }

    template <int N>
    nth_type_of<N, Args...>* get() {
        return reinterpret_cast<nth_type_of<N, Args...>*>(get_aligned());
    }

    int get_index() const {
        return get_bits();
    }


    template <int N, class... ConstructArgs>
    static EitherPtr2 construct(ConstructArgs&&... args) {
        using T = nth_type_of<N, Args...>;
        T* ptr = new (std::align_val_t(alignment())) T(std::forward<ConstructArgs>(args)...);
        void* raw = reinterpret_cast<void*>(ptr);
        WHEELS_VERIFY(reinterpret_cast<uintptr_t>(raw) % alignment() == 0, "Invalid alignment");

        EitherPtr2 p (raw);
        p.set_bits(N);
        return p;
    }

    explicit EitherPtr2(void* ptr)
        : ptr_(ptr)
    {}

    ~EitherPtr2() {
        destroy();
    }

private:

    void destroy() {

    }

    void set_bits(uintptr_t bits) {
        bits &= mask();
        uintptr_t as_int = reinterpret_cast<uintptr_t>(ptr_);
        as_int &= ~mask();
        as_int |= bits;
        ptr_ = reinterpret_cast<void*>(as_int);
    }

    uintptr_t get_bits() const {
        return reinterpret_cast<uintptr_t>(ptr_) & mask();
    }

    const void* get_aligned() const {
        uintptr_t as_int = reinterpret_cast<uintptr_t>(ptr_);
        return reinterpret_cast<void*>(as_int & ~mask());
    }

    void* get_aligned() {
        uintptr_t as_int = reinterpret_cast<uintptr_t>(ptr_);
        return reinterpret_cast<void*>(as_int & ~mask());
    }

    void* ptr_;
};


}

// namespace {

// template <int n_bits>
// constexpr int alignment() {
//     return 1 << n_bits;
// }

// template <int n_bits>
// constexpr uintptr_t mask() {
//     return (1 << n_bits) - 1;
// }

// }

// namespace five {

// template <typename T, int n_bits, typename... Args>
// std::shared_ptr<T> make_bit_ptr(Args&&... args) {
//     T* ptr = new (std::align_val_t(alignment<n_bits>())) T(std::forward(args...));
//     WHEELS_ASSERT(ptr % alignment<n_bits>() == 0, "invalid alignment");
//     return std::shared_ptr<T>(ptr);
// }

// // template <typename
// // uintptr_t get_bits()

// template <class T, int n_bits>
// class BitPtr : public std::shared_ptr<T> {
// public:
//     using std::shared_ptr<T>::shared_ptr;

//     int get_bits() const {
//         return static_cast<uintptr_t>(get()) & mask<n_bits>();
//     }

//     int set_bits(int bits) {
//         bits = bits & mask();
//         t_ &= ~static_cast<uintptr_t>(mask());
//         t_ |= bits;
//     }
// };

// template <class T, int n_bits, class... Args>
// BitPtr<T, n_bits> make_bit_ptr(Args&&... args) {
//     T* ptr = new (std::align_val_t(alignment<n_bits>())) T(std::forward(args...));
//     WHEELS_ASSERT(ptr % alignment<n_bits>() == 0, "invalid alignment");
//     std::make_shared<T>(ptr);
//     return std::shared_ptr<T>(ptr);
// }

// }
