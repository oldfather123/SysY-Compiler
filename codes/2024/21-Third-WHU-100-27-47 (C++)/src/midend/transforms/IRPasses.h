#pragma once

#include "IR.h"

namespace ir {
    void runIRPasses(Ptr<ir::Module> module, int optLevel = 1);
}
