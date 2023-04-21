#pragma once

#include <functional>
#include <memory>
#include <wheels/intrusive/list.hpp>
#include <five/xrange.h>
#include <deque>

namespace five {

template <typename T>
class ObjectPool {
    static_assert(std::is_base_of_v<wheels::IntrusiveListNode<T>, T>);

public:
    ObjectPool(size_t initial) {
        for (auto i : xrange(initial)) {
            WHEELS_UNUSED(i);
            list_.PushBack(new T);
        }
    }

    T* Get() {
        if (list_.NonEmpty()) {
            // NOTE: stack-like behaviour may improve cache-locality
            return list_.PopBack();
        }
        return new T;
    }

    void Put(T* t) {
        list_.pushBack(t);
    }

    void Clear() {
        while (!list_.empty()) {
            delete list_.PopFront();
        }
    }

    ~ObjectPool() {
        Clear();
    }

private:
    wheels::IntrusiveList<T> list_;
};


template <typename T>
class ObjectPoolLite {
public:
    using Factory = std::function<T()>;
    ObjectPoolLite(size_t initial, Factory&& factory)
        : objects_()
        , factory_(std::move(factory))
    {
        for (auto i : xrange(initial)) {
            WHEELS_UNUSED(i);
            objects_.emplace_back(std::move(factory_()));
        }
    }

    T get() {
        if (!objects_.empty()) {
            auto object = std::move(objects_.back());
            objects_.pop_back();
            return std::move(object);
        }
        return factory_();
    }

    void put(T&& t) {
        objects_.emplace_back(std::move(t));
    }

    void clear() {
        objects_.clear();
    }

    size_t size() const {
        return objects_.size();
    }

private:
    std::deque<T> objects_;
    Factory factory_;
};

}
