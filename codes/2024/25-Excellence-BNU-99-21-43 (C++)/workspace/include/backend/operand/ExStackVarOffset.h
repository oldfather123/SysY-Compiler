// ExStackVarOffset.h

#pragma once

#include "AsmOperandImmediate.h"
#include "ExStackVarAdd.h"

namespace Backend {

/**
 * @brief Stack offset, used for stack frame reallocation process.
 */
class ExStackVarOffset : public AsmOperandImmediate, public ExStackVarAdd {
public:
    ExStackVarOffset(int val);

    static ExStackVarOffset* transform(long value);

    AsmOperandBasic* add(int diff) override;
};

} // namespace Backend
