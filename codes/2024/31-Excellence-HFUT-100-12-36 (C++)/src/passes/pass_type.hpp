#ifndef PASS_TYPE
#define PASS_TYPE

namespace Passes {

enum PassType {
    MEM2REG,
    FUNCTION_INFO,
    DEAD_CODE_ELIMINATION,
    TAIL_CALL,
    LOOP_INVARIANTS
};

};

#endif