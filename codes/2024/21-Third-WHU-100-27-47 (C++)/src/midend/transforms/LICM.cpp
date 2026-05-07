#include "LICM.h"
#include "LoopDetection.h"
#include "CFA.h"
#include "DFA.h"
#include "UseDefAnalysis.h"
#include "LoopInvariantAnalysis.h"
#include "LoopIndVarAnalysis.h"

using namespace ir;

Vector<InstPtr> resolveDependencyOrder(const std::unordered_set<InstPtr> &insts, CFAContext &cfaCtx) {
    Vector<InstPtr> ordered{insts.begin(), insts.end()};
    std::sort(ordered.begin(), ordered.end(), [&](InstPtr a, InstPtr b) {
        return cfaCtx.getDFN(a) < cfaCtx.getDFN(b);
    });
    return ordered;
}

bool licmTrip(ir::FuncPtr func) {
    bool changed = false;

    CFAContext cfaCtx{func};
    LoopDetectionContext loopDetCtx{cfaCtx};
    DFAContext dfaCtx{func, DFAContext::RD | DFAContext::LV};
    UseDefAnalysisContext useDefCtx{dfaCtx};

    loopDetCtx.printDebug();

    for (auto loop : loopDetCtx.loops()) {
        // Loop invariant analysis
        LoopInvariantAnalysisContext loopInvarCtx{loop, cfaCtx, dfaCtx, useDefCtx};
        auto &invariantInsts = loopInvarCtx.invariantInsts;

        // Move loop invariants to the preheader
        if (!invariantInsts.empty()) {
            auto preheaderBB = loop->getOrCreatePreheaderBB();
            for (auto inst : resolveDependencyOrder(invariantInsts, cfaCtx)) {
                dbgout << "├── Moved loop invariant instruction from `" << inst->parentBB()->label() << "` to preheader `" << preheaderBB->label() << "`: " << inst->toString() << std::endl;
                Instruction::remove(inst);
                Instruction::addTo(inst, preheaderBB);
                Instruction::insertBefore(preheaderBB->exitInst(), inst);
                changed = true;
            }
        }
    }

    return changed;
}

bool ir::licm(ir::FuncPtr func) {
    dbgout << std::endl
           << "LICM pass started (" << func->name() << ")." << std::endl;

    bool changed = FixedPoint{
        [&](bool &_changed) {
            _changed |= licmTrip(func);
        },
        true,
        "Loop Invariant Code Motion",
        func->name(),
    };

    dbgout << "└── LICM pass done." << std::endl;

    return changed;
}
