#include <cstdint>

enum OpCode {
    OP_ASK = 1,
    OP_SET = 2,
    OP_DEL = 3,
};

enum RetCode {
    RET_OK = 1,
    RET_BAD_REQ = 2,
};
