#include "loopEliminate.h"
#include "Instruction.h"
#include "Type.h"
#include "domTreeAnalysis.h"
#include "Value.h"


bool runLoopEliminate(FunctionPtr func, DominateAnalysisResult& dom) {

    auto loopInfo = runLoopAnalysis(func, dom);
    bool modified = false;
    for(auto& loop : loopInfo.loops) {
        // innermost loop
        if(loop.header != loop.latch)
            continue;
        if(!loop.bound->is<Const>())
            continue;
        if(!loop.initVal->is<Const>())
            continue;
        const auto initial = loop.initVal->as<Const>()->intVal; // NOLINT
        const auto bound = loop.bound->as<Const>()->intVal;  // NOLINT
        // execute at most once
        if((loop.step > 0 && initial + loop.step > bound) || ((loop.step < 0 && initial + loop.step < bound))) {
            // remove backedge
            modified = true;
            const auto terminator = loop.latch->getTerminator()->as<BrInstruction>();
            terminator->getOperandsRef()[0]->replaceValue(Const::getConst(Type::getBool(), false));
        }
    }
    return modified;
}
