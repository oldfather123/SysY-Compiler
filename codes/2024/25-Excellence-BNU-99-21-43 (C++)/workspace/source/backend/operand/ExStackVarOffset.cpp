// ExStackVarOffset.cpp

#include "ExStackVarOffset.h"

namespace Backend {

ExStackVarOffset::ExStackVarOffset(int val) : AsmOperandImmediate(val) { setType(OperandType::EX_STACK_VAR_OFFSET); }

ExStackVarOffset* ExStackVarOffset::transform(long value) {
    return new ExStackVarOffset(static_cast<int>(value));
}

AsmOperandBasic* ExStackVarOffset::add(int diff) {
    if (getValue() < 0) {
        return new ExStackVarOffset(getValue() + diff);
    } else {
        return this;
    }
}

} // namespace Backend
