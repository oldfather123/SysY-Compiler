#pragma once

#include "Common.h"
#include "IR.h"
#include "DFA.h"

namespace ir {
    bool dce(ir::FuncPtr func, unsigned maxIter = INT_MAX);
}
