#include "UseDefAnalysis.h"

using namespace ir;

Set<Definition> UseDefAnalysisContext::getReachingDefs(Variable var, InstPtr targetInst) {
    dfaCtx.assertAvailable(DFAContext::RD);

    Set<ir::Definition> ret{};

    auto bb = targetInst->parentBB();
    auto reachingDefs = dfaCtx.getRDInSet(bb);
    for (auto inst : bb->getInstTopoList()) { // COMPLEXITY
        if (inst == targetInst) {
            break;
        }

        // kill
        for (auto mayDefVar : inst->mayDefVars()) {
            for (auto it = reachingDefs.begin(); it != reachingDefs.end();) { // COMPLEXITY
                auto reachingDef = *it;
                if (reachingDef.var.getBaseVar() == mayDefVar && reachingDef.var == mayDefVar) {
                    it = reachingDefs.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // gen
        for (auto mayDefVar : inst->mayDefVars()) {
            reachingDefs.insert(Definition{mayDefVar, inst});
        }
    }

    for (auto reachingDef : reachingDefs) {
        if (reachingDef.var == var) {
            ret.insert(reachingDef);
        }
    }
    return ret;
}

UseDefAnalysisContext::UseDefAnalysisContext(DFAContext &dfaCtx) : dfaCtx(dfaCtx) {
    func = dfaCtx.func;

    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstTopoList()) {
            for (auto defVar : inst->mustDefVars()) {
                defInstMap[defVar].insert(inst);
            }
        }
    }

    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstTopoList()) {
            for (auto useVar : inst->mustUseVars()) {
                for (auto defInst : defInstMap[useVar]) {
                    userMap[defInst].insert(inst);
                }
            }
        }
    }

    dbgout << "├── UseDefAnalysisContext constructed for function " << func->name() << "." << std::endl;
}
