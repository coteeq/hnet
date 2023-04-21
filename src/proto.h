#include "net.h"
#include <functional>
#include <optional>

template <typename T>
net::Buf serialize(const T&);

template <typename T>
T deserialize(const net::Buf&);


class Protocol {
public:
    using Cookie = ssize_t;
    using Callback = std::function<net::Buf(const net::Buf&)>;

    virtual Cookie call(size_t idx) = 0;

    void register_cb(size_t idx, Callback&& cb);

private:
    std::vector<std::optional<Callback>> callbacks_;
};
