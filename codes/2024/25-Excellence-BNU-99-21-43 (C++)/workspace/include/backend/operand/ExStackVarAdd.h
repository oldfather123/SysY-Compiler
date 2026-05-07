// ExStackVarAdd.h

#pragma once

#include "AsmOperandBasic.h"

namespace Backend {

class ExStackVarAdd {
public:
    virtual AsmOperandBasic* add(int diff) = 0;
};

} // namespace Backend
