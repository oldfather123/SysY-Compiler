#pragma once

#include "Common.h"
#include "IR.h"
#include "DFA.h"

namespace ir {
    struct UseDefAnalysisContext {
    private:
        Map<ir::Variable, Set<ir::InstPtr>> defInstMap{};
        Map<ir::InstPtr, Set<ir::InstPtr>> userMap{};

    public:
        ir::DFAContext &dfaCtx;
        ir::FuncPtr func = nullptr;

        const Set<ir::InstPtr> &getDefInsts(ir::Variable var) {
            return defInstMap[var];
        }

        const Set<ir::InstPtr> &getUsers(ir::InstPtr inst) {
            return userMap[inst];
        }

        Set<ir::Definition> getReachingDefs(ir::Variable var, ir::InstPtr targetInst);

        Set<ir::InstPtr> getDataFlowDependencies(ir::InstPtr inst) {
            Set<ir::InstPtr> ret{};
            for (auto use : inst->mustUseVars()) {
                for (auto reachingDef : getReachingDefs(use, inst)) {
                    ret.insert(reachingDef.inst);
                }
            }
            return ret;
        }

        UseDefAnalysisContext(ir::DFAContext &dfaCtx);
        UseDefAnalysisContext(const UseDefAnalysisContext &other) = delete;
    };
}
