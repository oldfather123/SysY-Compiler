#pragma once

#include "Common.h"
#include "IR.h"

namespace ir {
    void strengthReduce(ir::FuncPtr func, bool introduceBitwiseOperations = false);
}
