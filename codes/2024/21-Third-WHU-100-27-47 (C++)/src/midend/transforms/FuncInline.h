#pragma once

#include "Common.h"
#include "IR.h"

namespace ir {
    bool funcInline(ir::FuncPtr func, unsigned maxIter = 1);
}
