#include "LoopInvariantAnalysis.h"

using namespace ir;

ir::LoopInvariantAnalysisContext::LoopInvariantAnalysisContext(Ptr<Loop> loop, CFAContext &cfaCtx, DFAContext &dfaCtx, UseDefAnalysisContext &useDefCtx) : loop(loop), cfaCtx(cfaCtx), dfaCtx(dfaCtx), useDefCtx(useDefCtx) {
    dfaCtx.assertAvailable(DFAContext::LV);

    Set<InstPtr> loopInsts{};

    // Collect all instructions in the loop
    for (auto bb : loop->bbs()) {
        for (auto inst : bb->getInstSet()) {
            loopInsts.insert(inst);
        }
    }

    // Mark loop invariants
    FixedPoint{
        [&](bool &_changed) {
            for (auto bb : loop->bfsBBList()) {
                for (auto inst : bb->getInstTopoList()) {
                    if (inst == bb->entryInst() || inst == bb->exitInst() || inst->is<AllocInst>() || inst->is<PhiInst>() || inst->is<CallInst>()) {
                        continue;
                    }
                    bool isInvariantInst = true;
                    // Only consider variables as operands, because constants are invariants already
                    for (auto operand : inst->mayUseVars()) {
                        // If operand is a global variable pointer or a register argument, then operand is invariant
                        if ((operand.isReg() && operand.getReg()->isGlobal()) || (operand.isReg() && operand.getReg()->isArg())) {
                            _changed |= invariantOperands.insert(operand).second;
                            continue;
                        }
                        // If all reaching definitions of operand are outside the loop, then operand is invariant
                        auto reachingDefs = useDefCtx.getReachingDefs(operand, inst);
                        bool allReachingDefsAreOutsideLoop = false;
                        if (!reachingDefs.empty()) {
                            allReachingDefsAreOutsideLoop = true;
                            for (auto reachingDef : reachingDefs) {
                                if (loopInsts.find(reachingDef.inst) != loopInsts.end()) {
                                    allReachingDefsAreOutsideLoop = false;
                                }
                            }
                        }
                        if (allReachingDefsAreOutsideLoop) {
                            _changed |= invariantOperands.insert(operand).second;
                            continue;
                        }
                        // If operand has only one reaching definition inside the loop and the definition is marked as invariant, then operand is invariant
                        if (reachingDefs.size() == 1 && invariantInsts.count(reachingDefs.begin()->inst)) {
                            _changed |= invariantOperands.insert(operand).second;
                            continue;
                        }
                        // Operand is not invariant, so inst is not invariant
                        isInvariantInst = false;
                        break;
                    }

                    // Check whether it can be moved
                    // For every def variable of inst
                    for (auto mayDefVar : inst->mayDefVars()) {
                        // Requirement 1
                        for (auto exitEdge : loop->exitEdges()) {
                            auto loopExitBB = exitEdge->src();
                            if (dfaCtx.getLVOutSet(loopExitBB).count(mayDefVar)) {
                                if (!cfaCtx.dominates(inst, loopExitBB->exitInst())) {
                                    isInvariantInst = false;
                                    break;
                                }
                            }
                        }
                        if (!isInvariantInst) {
                            break;
                        }

                        // Requirement 3
                        if (dfaCtx.getLVInSet(loop->headerBB()).count(mayDefVar)) {
                            isInvariantInst = false;
                            break;
                        }
                    }
                    if (isInvariantInst) {
                        bool mark = invariantInsts.insert(inst).second;
                        if (mark) {
                            dbgout << "├── Marked loop invariant instruction: " << inst->toString() << std::endl;
                        }
                        _changed |= mark;
                    }
                }
            }
        },
        true,
        "Loop Invariants Marking",
        loop->headerBB()->label(),
    };

    dbgout << "├── LoopInvariantsAnalysisContext constructed for loop " << loop->headerBB()->label() << "." << std::endl;
}
