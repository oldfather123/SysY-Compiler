// NOLINTBEGIN
#include "riscv.h"

// Optimization passes
#include "algebraicIdentity.h"
#include "doubleBandwidth.h"
#include "instructionReorder.h"
#include "peepholePass.h"
#include "registerAllocation.h"
#include "removeIdenticalMoves.h"
#include "removeUnusedInstruction.h"
#include "replaceBlockDFS.h"
#include "useAnalysis.h"

#define RUN(passName) passName(programAsm)
void optimizeMachineIR(MProg* programAsm) {
    RUN(replaceBlockDFS);
    stillOptimize = true;
    while (stillOptimize) {
        stillOptimize = false;
        RUN(algebraicIdentity);
        RUN(useAnalysis);
        RUN(peepholePass);
        RUN(doubleBandwidth);
        RUN(removeUnusedInstruction);
        RUN(useAnalysis);
    }
    registerAllocation(programAsm, false);
    RUN(removeIdenticalMoves);
    RUN(instructionReorder);
    printf("ok\n");
}
#undef RUN