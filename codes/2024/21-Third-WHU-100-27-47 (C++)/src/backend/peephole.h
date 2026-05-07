#ifndef BACKEND_PEEPHOLE_H_
#define BACKEND_PEEPHOLE_H_

#include "Common.h"
#include "machine_code.h"
#include "instruction.h"
#include "IR.h"

namespace backend {
    void optimizeWindow(std::vector<Ptr<Instruction>> &, Ptr<RiscBasicBlock> &, bool &);

    void peephole(Ptr<RiscFunction>, bool &);
}

#endif
