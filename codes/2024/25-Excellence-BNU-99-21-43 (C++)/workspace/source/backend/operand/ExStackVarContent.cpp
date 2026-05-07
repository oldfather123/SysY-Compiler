// ExStackVarContent.cpp

#include "ExStackVarContent.h"

namespace Backend {

ExStackVarContent::ExStackVarContent(AsmOperandMemoryAddress* address, int size, bool isS0Based)
    : AsmOperandStackVariable(address, size, isS0Based) { setType(OperandType::EX_STACK_VAR_CONTENT); }

ExStackVarContent* ExStackVarContent::transform(AsmOperandStackVariable* stackVar) {
    return new ExStackVarContent(stackVar->getAddress(), stackVar->getSize(), stackVar->isS0Based());
}

ExStackVarContent* ExStackVarContent::transform(AsmOperandStackVariable* stackVar, AsmOperandMemoryAddress *address) {
    return new ExStackVarContent(address, stackVar->getSize(), stackVar->isS0Based());
}

AsmOperandBasic* ExStackVarContent::add(int diff) {
    if (isS0Based() && getAddress()->getOffset() < 0) {
        return new ExStackVarContent(getAddress()->addOffset(diff), getSize(), true);
    } else {
        return this;
    }
}

AsmOperandBasic* ExStackVarContent::withRegister(AsmOperandRegister* reg) const {
    return new ExStackVarContent(static_cast<AsmOperandMemoryAddress*>(getAddress()->withRegister(reg)), getSize(), isS0Based());
}

} // namespace Backend
