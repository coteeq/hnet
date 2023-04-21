#include "xrange.h"

namespace five {

Xrange xrange(i64 stop) {
    return Xrange(0, stop, 1);
}

Xrange xrange(i64 start, i64 stop) {
    return Xrange(start, stop, 1);
}

Xrange xrange(i64 start, i64 stop, i64 step) {
    return Xrange(start, stop, step);
}

}
