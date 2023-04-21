#include "proto.h"

using net::Buf;

template<>
Buf serialize(const std::string& s) {
    return Buf();
}

template<>
Buf serialize(const size_t& s) {
    Buf buf(20);
    sprintf(buf.data_ptr(), "%zu", s);
    return buf;
}

template <>
std::string deserialize(const Buf& buf) {
    return std::string(buf.data_ptr(), buf.len());
}

template <>
size_t deserialize(const Buf& buf) {
    return strtoull(buf.data_ptr(), nullptr, 10);
}
