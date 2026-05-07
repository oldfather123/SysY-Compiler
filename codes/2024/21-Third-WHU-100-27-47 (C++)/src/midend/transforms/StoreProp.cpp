#include "StoreProp.h"
#include "DFA.h"
#include "UseDefAnalysis.h"

using namespace ir;

bool ir::storeProp(FuncPtr func) {
    bool changed = false;

    dbgout << std::endl
           << "StoreProp pass started (" << func->name() << ")." << std::endl;

    DFAContext dfaCtx{func, DFAContext::RD};
    UseDefAnalysisContext useDefCtx{dfaCtx};
    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            if (auto loadInst = inst->as<LoadInst>()) {
                auto memVar = Variable::mem(loadInst->srcAddrReg());
                auto reachingDefs = useDefCtx.getReachingDefs(memVar, loadInst); // COMPLEXITY
                if (reachingDefs.size() == 1) {
                    auto reachingDef = *reachingDefs.begin();
                    if (auto srcStoreInst = reachingDef.inst->as<StoreInst>()) {
                        auto moveInst = MoveInst::create(bb, loadInst->destReg(), srcStoreInst->srcVal());
                        dbgout << "├── Replaced load `" << loadInst->toString() << "` with `" << moveInst->toString() << "`" << std::endl;
                        Instruction::replace(loadInst, moveInst);
                        changed = true;
                    }
                }
            }
        }
    }

    dbgout << "└── StoreProp pass done." << std::endl;
    return changed;
}
