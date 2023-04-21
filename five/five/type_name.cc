#include "type_name.h"
#include <cxxabi.h>

namespace five {

std::string demangle(const std::string& mangled) {
    int status;
    char *ret = abi::__cxa_demangle(mangled.c_str(), 0, 0, &status);

    /* NOTE: must free() the returned char when done with it! */
    std::string result(ret);
    free(ret);
    return result;
}

}
