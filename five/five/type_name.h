#pragma once
#include <typeinfo>
#include <string>

namespace five {

std::string demangle(const std::string& mangled);

template <typename T>
std::string type_name() {
    return demangle(typeid(T).name());
}

}
