#include "Function.h"
#include "Instruction.h"
#include "Loop.h"
#include "Value.h"
#include "domTreeAnalysis.h"
#include "loopAnalysis.h"
#include "patternMatch.h"

#include <cassert>
#include <memory>

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Analysis/LoopAnalysis.cpp#L4

LoopAnalysisResult runLoopAnalysis(FunctionPtr func, DominateAnalysisResult& dom) {

    // TODO: clear loop storages

    vector<Loop> loops;

    for(auto bb : dom.blocks()) {
        // find the conditional branch bb
        auto terminator = bb->getTerminator();
        if(terminator->getInsID() != InsID::Br) {
            continue;
        }
        auto br = dynamic_cast<BrInstruction*>(terminator);
        auto cond = br->getCondition();
        if(cond == nullptr)
            continue;
        // FIXME: check if the condition is a icmp instruction
        //  the doc don't exclude the FCMP
        auto icmp = dynamic_cast<IcmpInstruction*>(cond);
        if(icmp == nullptr)
            continue;
        // while ( i < bound)
        // i = i + n
        // get the next (i)  and the bound
        auto next = icmp->getOperand(0);
        auto bound = icmp->getOperand(1);

        Value* indVar;
        int step;
        // FIXME: assert all is add a constant(we have sub 'sub' to 'add')
        //  but the doc don't exclude the 'mul' and 'div', like i = i * 2
        //  so we don't deal with 'mul' and 'div' now, "just  not seen as a loop"
        //  after stay right, the const must be the second operand

        if(!add(any(indVar), int_(step))({ MatchContext<Value>{ next } })) {
            continue;
        }

        auto trueTarget = br->trueTarget;
        auto header = trueTarget;
        if(!dom.dominate(header, bb))
            continue;
        if(indVar->getBasicBlock() != header || !indVar->is<PhiInstruction>())
            continue;

        //! the bound should be loop invariant
        // FIXME:
        if(!bound->isConst) {
            BasicBlockPtr bb = nullptr;
            if(bound->valueId() == ValueID::Variable) {
                bb = func->getEntryBlock();
            } else
                bb = bound->getBasicBlock();
            assert(bb != nullptr);
            if(bb == header || !dom.dominate(bb, header)) {
                continue;
            }
        }

        Value* initVal = nullptr;
        bool uniqueInitial = true;
        const auto phi = indVar->as<PhiInstruction>();
        for(auto [pred, use] : phi->incomings()) {
            if(pred != bb) {
                if(initVal)
                    uniqueInitial = false;
                else
                    initVal = use->val;
            }
        }

        if(!uniqueInitial || !initVal)
            continue;

        if(phi->incomings().at(bb)->val != next)
            continue;

        if(icmp->kind == IcmpKind::ICmpNE) {
            int boundValue;
            if(!int_(boundValue)(MatchContext<Value>{ bound }))
                continue;
            int initialValue;
            if(!int_(initialValue)(MatchContext<Value>{ initVal }))
                continue;
            if((boundValue - initialValue) % step)
                continue;
            if(step > 0 && (initialValue >= boundValue))
                continue;
            if(step < 0 && (initialValue <= boundValue))
                continue;
        } else if(icmp->kind == IcmpKind::ICmpSLT) {
            // increment
            if(step <= 0)
                continue;
        } else if(icmp->kind == IcmpKind::ICmpSGT) {
            // decrement
            if(step >= 0)
                continue;
        } else if(icmp->kind == IcmpKind::ICmpSLE) {
            if(step <= 0)
                continue;
        } else if(icmp->kind == IcmpKind::ICmpSGE) {
            if(step >= 0)
                continue;
        } else {
            continue;
        }

        loops.emplace_back(header, bb, indVar, next, initVal, bound, step);
    }

    return { std::move(loops) };

    // computeExitingBlocks(block);
    // computeExitBlocks(block);
    // computeLatch(block);
    // computePreHeader(block);
    // computeLoopIndVar(block);
    // computeLoopBB(block);
}
// NOLINTEND
