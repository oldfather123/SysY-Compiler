#include "Block.h"
#include "Instruction.h"
#include "riscv.h"
#include "loopCanonicalize.h"


//static bool simplifyIndVar(BasicBlock* block, IcmpInstruction* cmp, BrInstruction* terminatorBranch,
//                           const DominateAnalysisResult& dom) {
//    // addrec (initial, step) + offset < end -> addrec (initial, step) + step < end + step - offset
//    const auto next = cmp->getOperand(0);
//    const auto rhs = cmp->getOperand(1);
//    Value* v1;
//    intmax_t i1, i2;
//    if(!(add(any(v1), int_(i1))(MatchContext<Value>{ next }) && std::abs(i1) < maxStep))
//        return false;
//    if(cmp->getOp() != CompareOp::ICmpSignedLessThan && cmp->getOp() != CompareOp::ICmpSignedLessEqual &&
//       cmp->getOp() != CompareOp::ICmpSignedGreaterThan && cmp->getOp() != CompareOp::ICmpSignedGreaterEqual)
//        return false;
//
//    if(!v1->is<PhiInst>())
//        return false;
//    const auto phi = v1->as<PhiInst>();
//    const auto latch = block;
//    const auto header = terminatorBranch->getTrueTarget();
//    if(!(phi->getBlock() == header && phi->incomings().count(latch)))
//        return false;
//    const auto indvar = phi->incomings().at(latch)->value;
//    if(!(add(exactly(v1), int_(i2))(MatchContext<Value>{ indvar }) && i1 != i2 && std::abs(i2) < maxStep))
//        return false;
//    if(rhs->getBlock() != nullptr && !(rhs->getBlock() != block && dom.dominate(rhs->getBlock(), block)))
//        return false;
//    const auto newLhs = make<BinaryInst>(InstructionID::Add, v1, ConstantInteger::get(v1->getType(), i2));
//    const auto newRhs = make<BinaryInst>(InstructionID::Add, rhs, ConstantInteger::get(rhs->getType(), i2 - i1));
//    newLhs->insertBefore(block, cmp->asIterator());
//    newRhs->insertBefore(block, cmp->asIterator());
//    cmp->mutableOperands()[0]->resetValue(newLhs);
//    cmp->mutableOperands()[1]->resetValue(newRhs);
//    return true;
//}

bool runLoopCanonicalize(FunctionPtr func) {
    auto cfg = runCFGAnalysis(func);
    auto dom = runDominateTreeAnalysis(func, cfg);
    bool modified = false;

    for(auto& block : func->basicBlocks) {
        const auto terminator = block->getTerminator();
        if(terminator->insId != InsID::Br)
            continue;
        // assert icmp and cond branch
        if (terminator->getNumOperands() == 0) continue;
        auto cond = terminator->getOperand(0);
        if (cond == nullptr) continue;
        if(!cond->is<IcmpInstruction>())
            continue;
        auto cmp = cond->as<IcmpInstruction>();
        // make constants rhs
        if(cmp->getOperand(0)->isConst && !cmp->getOperand(1)->isConst) {
            auto& operands = cmp->getOperandsRef();
            std::swap(operands[0], operands[1]);
            cmp->setReverseKind();
            modified = true;
        }

        auto terminatorBranch = terminator->as<BrInstruction>();
        if(terminatorBranch->getTrueTarget() != terminatorBranch->getFalseTarget() && cmp->hasOnlyOneUse() &&
           dom.dominate(terminatorBranch->getFalseTarget(), block.get()) && !dom.dominate(terminatorBranch->getTrueTarget(), block.get())) {
            terminatorBranch->swapTargets();
            cmp->setInvertedKind();
            modified = true;
        }


        //if(terminatorBranch->getTrueTarget() != terminatorBranch->getFalseTarget() && cmp->hasExactlyOneUse() &&
        //   dom.dominate(terminatorBranch->getTrueTarget(), block) &&
        //   (!dom.dominate(terminatorBranch->getFalseTarget(), block) ||
        //    dom.dominate(terminatorBranch->getFalseTarget(), terminatorBranch->getTrueTarget()))) {
        //    // TODO: check overflow
        //    // i <= n -> i < n + 1
        //    if(cmp->getOp() == CompareOp::ICmpSignedLessEqual) {
        //        const auto next = cmp->getOperand(0);
        //        PhiInst* indVar;
        //        intmax_t step;
        //        if(add(phi(indVar), int_(step))(MatchContext<Value>{ next })) {
        //            const auto bound = cmp->getOperand(1);
        //            // constant bound will be handled by ArithmeticReduce
        //            if(!bound->isConstant() &&
        //               (!bound->getBlock() ||
        //                (bound->getBlock() != indVar->getBlock() && dom.dominate(bound->getBlock(), indVar->getBlock())))) {
        //                const auto newBound =
        //                    make<BinaryInst>(InstructionID::Add, bound, ConstantInteger::get(bound->getType(), 1));
        //                if(bound->getBlock()) {
        //                    auto it = std::next(bound->as<Instruction>()->asIterator());
        //                    while(it->getInstID() == InstructionID::Phi)
        //                        ++it;
        //                    newBound->insertBefore(bound->getBlock(), it);
        //                } else {
        //                    for(auto& inst : func.entryBlock()->instructions())
        //                        if(inst.getInstID() != InstructionID::Alloc) {
        //                            newBound->insertBefore(func.entryBlock(), std::next(inst.asIterator()));
        //                            break;
        //                        }
        //                }
        //                cmp->mutableOperands()[1]->resetValue(newBound);
        //                cmp->setOp(CompareOp::ICmpSignedLessThan);
        //                modified = true;
        //            }
        //        }
        //    }
        //}

        //modified |= simplifyIndVar(block, cmp, terminatorBranch, dom);
    }

    return modified;
}
