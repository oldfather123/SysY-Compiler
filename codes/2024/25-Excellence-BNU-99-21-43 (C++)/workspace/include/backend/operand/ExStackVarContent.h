// ExStackVarContent.h

#pragma once

#include "AsmOperandStackVariable.h"
#include "ExStackVarAdd.h"

namespace Backend {

/**
 * @brief Stack variable content, used for stack frame reallocation process.
 */
class ExStackVarContent : public AsmOperandStackVariable, public ExStackVarAdd {
private:
    ExStackVarContent(AsmOperandMemoryAddress* address, int size, bool isS0Based);

public:
    static ExStackVarContent* transform(AsmOperandStackVariable* stackVar);
    static ExStackVarContent* transform(AsmOperandStackVariable* stackVar, AsmOperandMemoryAddress* address);

    AsmOperandBasic* add(int diff) override;

    AsmOperandBasic* withRegister(AsmOperandRegister* reg) const override;
};

} // namespace Backend
