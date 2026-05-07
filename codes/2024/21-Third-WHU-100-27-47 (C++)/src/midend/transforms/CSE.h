#pragma once
#include "CFA.h"
#include "DFA.h"
#include "Common.h"

namespace ir {
    bool cse(ir::FuncPtr func, unsigned maxIter = INT_MAX);
}
