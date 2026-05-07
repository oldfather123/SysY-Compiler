
#include "Block.h"
// #include "CFGAnalysis.h"
#include "Instruction.h"
#include "Loop.h"
#include "Value.h"
#include <Function.h>
#include <IRbuilder.h>
#include <cassert>
#include <memory>
#include <utility>

void fixPhiNode(BasicBlock* oldPred, BasicBlock* newPred) {
    const auto terminator = newPred->getTerminator();
    if(terminator->isBranch()) {
        auto handleTarget = [&](BasicBlock* target) {
            for(auto& phiInst : target->instructionsRef()) {
                if(phiInst->insId == InsID::Phi) {
                    phiInst.get()->as<PhiInstruction>()->replaceSource(oldPred, newPred);  // NOLINT
                } else
                    break;
            }
        };
        applyForSuccessors(terminator, handleTarget);
    }
}
void applyForSuccessors(Instruction* branchOrSwitch, const std::function<void(BasicBlock*)>& functor) {
    // immutable
    const auto branch = branchOrSwitch->as<BrInstruction>();
    const auto trueTarget = branch->getTrueTarget();
    const auto falseTarget = branch->getFalseTarget();
    functor(trueTarget);
    if(falseTarget && trueTarget != falseTarget)
        functor(falseTarget);
}

void resetTarget(Instruction* brInst, BasicBlockPtr oldTarget, BasicBlockPtr newTarget) {

    const auto branch = brInst->as<BrInstruction>();
    assert(branch->getTrueTarget() == oldTarget || branch->getFalseTarget() == oldTarget);
    if(branch->getTrueTarget() == oldTarget) {
        branch->trueTarget = newTarget;
    }
    if(branch->getFalseTarget() == oldTarget) {
        branch->falseTarget = newTarget;
    }
}

void retargetBlock(BasicBlockPtr target, BasicBlockPtr oldSource, BasicBlockPtr newSource) {
    for(auto& inst : target->instructionsRef()) {
        if(inst->insId == InsID::Phi) {
            const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
            if(phi->incomings().count(newSource)) {
                assert(phi->incomings().at(oldSource) == phi->incomings().at(newSource));
                phi->removeSource(oldSource);
            } else
                phi->replaceSource(oldSource, newSource);
        } else
            break;
    }
}

// split 为两个blcok， 分割指令在第一个block中
// TODO: 修改获取迭代器的方式，现在方式性能很差, 一直需要find，更好的方式是直接获取迭代器
// TODO: 未经过测试
BasicBlockPtr splitBasicBlock(Instruction* after) {
    auto preBb = after->getBasicBlock();
    auto func = preBb->belongfunc;
    // auto nextBb = std::make_unique<BasicBlock>("split."+preBb->getName());
    auto nextBb = std::make_unique<BasicBlock>("split." + preBb->getName());
    auto res = nextBb.get();
    nextBb->setBelongFunc(func);
    // FIXME: 性能很差
    auto& oldIns = preBb->instructionsRef();
    // auto iter = after->getIterator();
    auto iter = oldIns.begin();
    for(iter = oldIns.begin(); iter != oldIns.end(); iter++) {
        if(iter->get() == after) {
            break;
        }
    }

    auto begin = std::next(iter);
    auto end = oldIns.end();
    auto& newIns = nextBb->instructionsRef();
    for(auto iter = begin; iter != end;) {
        auto next = std::next(iter);
        // moveBefore(nextBb.get(), newIns.begin(), *iter);
        (*iter)->setBasicBlock(res);
        newIns.push_back(std::move(*iter));
        iter = next;
    }
    // clean moved old instructions
    oldIns.erase(begin, end);

    // push terminator inst to preBb
    auto brInst = std::make_unique<BrInstruction>(nextBb.get());
    brInst->setBasicBlock(preBb);

    preBb->forceSetEndInst(std::move(brInst));

    // insert new block after preBb
    auto iter2 = std::find_if(func->basicBlocks.begin(), func->basicBlocks.end(),
                              [preBb](std::unique_ptr<BasicBlock>& ptr) -> bool { return ptr.get() == preBb; });
    func->basicBlocks.insert(std::next(iter2), std::move(nextBb));

    for(auto& bb : func->basicBlocks) {
        for(auto& inst : bb->instructionsRef()) {
            if(inst->insId == InsID::Phi) {
                auto phi = dynamic_cast<PhiInstruction*>(inst.get());
                phi->replaceSource(preBb, res);
            }
        }
    }

    return res;
    // assert(false);
}

unique_ptr<BasicBlock> BasicBlock::clone(std::unordered_map<Value*, Value*>& vmap, string prefix) const {
    auto bb = std::make_unique<BasicBlock>(getStr());
    bb->belongfunc = belongfunc;
    // TODO: set end instruction and beforeEndIns，似乎中后端没有用到这两个属性

    for(auto& inst : mInstructions) {
        auto newInst = inst->clone();
        if(inst->insId != InsID::Phi) {
            for(auto& operand : newInst->getOperandsRef()) {
                // FIXME: for phi
                if(operand->val->getBasicBlock() != this)
                    continue;
                if(auto iter = vmap.find(operand->val); iter != vmap.end()) {
                    operand->replaceValue(iter->second);
                }
            }
        }
        vmap.emplace(inst.get(), newInst);
        // FIXME: 还未经过测试
        insertBefore(bb.get(), bb->mInstructions.end(), newInst);
    }

    return std::move(bb);
}

unique_ptr<BasicBlock> BasicBlock::mClone(std::unordered_map<Value*, Value*>& vmap, std::string prefix) const {
    auto bbPtr = std::make_unique<BasicBlock>(prefix + getStr());
    auto bb = bbPtr.get();
    bb->belongfunc = belongfunc;
    // TODO: set end instruction and beforeEndIns，似乎中后端没有用到这两个属性

    for(auto& inst : mInstructions) {
        auto newInst = inst->clone();
        newInst->setBasicBlock(bb);
        for(auto& operand : newInst->getOperandsRef()) {
            // FIXME: for phi
            if(auto iter = vmap.find(operand->val); iter != vmap.end()) {
                operand->replaceValue(iter->second);
            }
        }
        vmap.emplace(inst.get(), newInst);
        // FIXME: 还未经过测试
        insertBefore(bb, bb->mInstructions.end(), newInst);
    }

    return std::move(bbPtr);
}
// solve phi inst with only one pred
void solvePhiInstruction(BasicBlockPtr bb) {
    assert(bb->predBasicBlocks.size() == 1);
    auto pred = bb->predBasicBlocks[0];
    for(int i = 0; i < bb->getBlockSize(); i++) {
        auto& inst = bb->instructionsRef()[i];
        if(inst->insId == InsID::Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(inst.get());
            auto val = phi->mIncomings[pred]->val;
            phi->replaceWith(val);
            phi->eraseFromParent();
            i--;
        }
    }
}
// 保留basicblock a; a的后继只有b； b的前驱只有a
void mergeBasicBlock(BasicBlock* a, BasicBlock* b) {
    if(b->instructionsRef().front()->insId == InsID::Phi) {
        solvePhiInstruction(b);
    }
    assert(a->belongfunc == b->belongfunc);
    assert(a->getSuccessor().size() == 1);
    assert(b->getPredecessor().size() == 1);
    assert(a->getSuccessor()[0] == b);
    a->instructionsRef().pop_back();
    // 获取 b 中的所有指令
    vector<unique_ptr<Instruction>> bIns;
    bIns.reserve(b->instructionsRef().size());
    for(auto& ins : b->instructionsRef()) {
        bIns.push_back(std::move(ins));
    }
    b->instructionsRef().clear();
    // 将 b 中的所有指令移动到 a 中
    for(auto& ins : bIns) {
        ins->setBasicBlock(a);
        a->instructionsRef().push_back(std::move(ins));
    }
    a->succBasicBlocks.clear();
    a->succBasicBlocks = b->succBasicBlocks;
    // 替换b的后继中的前驱b为a
    for(auto& succ : b->getSuccessor()) {
        succ->replacePredecessor(b, a);
    }
    b->succBasicBlocks.clear();
    // FIXME: 这里可能丢失returnbasicblock 信息  如果后面opt用到returnbasicblock 会有问题
    if(b->belongfunc->returnBasicBlock == b) {
        b->belongfunc->returnBasicBlock = a;
    }
    b->eraseFromParent();
}

void BasicBlock::eraseFromParent() {
    // FIXME:
    auto func = belongfunc;
    auto iter = std::find_if(func->basicBlocks.begin(), func->basicBlocks.end(),
                             [this](std::unique_ptr<BasicBlock>& ptr) -> bool { return ptr.get() == this; });
    if(iter != func->basicBlocks.end()) {
        func->basicBlocks.erase(iter);
    }

}

BasicBlock* BasicBlock::releaseFromParent() {
    auto func = belongfunc;
    auto iter = std::find_if(func->basicBlocks.begin(), func->basicBlocks.end(),
                             [this](std::unique_ptr<BasicBlock>& ptr) -> bool { return ptr.get() == this; });
    if (iter != func->basicBlocks.end()) {
        auto res = iter->release();
        func->basicBlocks.erase(iter);
        return res;
    }
    return nullptr;
}


// bool reduceBlock(IRbuilder& builder, BasicBlockPtr block, const BlockReducer& reducer) {
//     auto& instructions = block->instructionsRef();
//     bool modified = false;
//     auto oldSize = instructions.size();
//     for(auto it = instructions.begin(); it != instructions.end(); ++it) {
//         auto inst = *it;
//         builder.setInsertPoint(inst);
//         if(auto value = reducer(inst)) {
//             assert(value != inst);
//             modified |= inst->replaceWith(value);
//         }
//     }
//     const auto newSize = block->instructionsRef().size();
//     modified |= newSize != oldSize;
//     return modified;
// }

// 用replace_map替换inst中的操作数
//`bool applyReplace(InstructionPtr inst, const ReplaceMap& replace) {
//`    bool modified = false;
//`    for(auto& ref : inst->getOperandsRef()) {
//`        if(auto iter = replace.find(ref->val); iter != replace.end()) {
//`            ref->replaceValue(iter->second);
//`            modified = true;
//`        }
//`    }
//`    return modified;
//`}

void BasicBlock::forceSetEndInst(std::unique_ptr<Instruction> inst) {
    if(!instructionsRef().empty())
        beforeEndIns = instructionsRef().back().get();

    if(endInstruction) {
        // belongfunc->instToRemove.push_back(endInstruction);
        if(instructionsRef().back().get() == endInstruction && instructionsRef().size() >= 2)
            beforeEndIns = instructionsRef()[instructionsRef().size() - 2].get();
    }

    endInstruction = inst.get();
    endInstruction->setBasicBlock(this);
    instructionsRef().push_back(std::move(inst));
}

void BasicBlock::dump(std::ostream& out) {
    out << label.name() << ": ";
    int cnt = 0, n = predBasicBlocks.size();  // NOLINT
    for(auto& pred : predBasicBlocks) {
        if(cnt == 0) {
            out << "\t\t\t; preds = ";
        }
        out << "%" + pred->getStr();
        if(cnt < n - 1) {
            out << ", ";
        }
        cnt++;
    }
    out << endl;
    for(auto& instruction : instructions()) {
        instruction->dump(out);
    }
}

bool BasicBlock::setEndInstruction(std::unique_ptr<Instruction> inst) {
    if(!endInstruction) {
        endInstruction = inst.get();
        if(!instructionsRef().empty())
            beforeEndIns = instructionsRef().back().get();
        instructionsRef().push_back(std::move(inst));
        endInstruction->setBasicBlock(this);
        return true;
    }
    // belongfunc->instToRemove.push_back(inst);
    // instructionsRef().push_back(inst);
    // inst->setBasicBlock(shared_ptr<BasicBlock>(this));
    return false;
}

// void Block::clearBBToLoop() {
//     bbToLoop.clear();
//     for(const auto& basicBlock : basicBlocks) {
//         basicBlock->loop = nullptr;
//         basicBlock->loopDepth = 0;
//     }
// }
//
// void Block::clearLoops() {
//     loops.clear();
//     topLoops.clear();
// }
//

// NOLINTBEGIN
void BasicBlock::basicBlockDFSPost(BasicBlock* bb, function<bool(BasicBlock*)> cond, function<void(BasicBlock*)> action) {
    if(cond(bb))
        return;
    for(const auto& succ : bb->getSuccessor())
        basicBlockDFSPost(succ, cond, action);
    action(bb);
}

void BasicBlock::replacePhiSource(BasicBlock* oldPred, BasicBlock* newPred) {
    for(auto& instr : instructionsRef()) {
        if(instr->insId == InsID::Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
            phi->replaceSource(oldPred, newPred);
        }
    }
}

void BasicBlock::replacePredecessor(BasicBlock* oldPred, BasicBlock* newPred) {
    if(find(predBasicBlocks.begin(), predBasicBlocks.end(), newPred) != predBasicBlocks.end()) {
        deletePredBasicBlock(oldPred);
        return;
    }
    auto iter = std::find(predBasicBlocks.begin(), predBasicBlocks.end(), oldPred);
    if(iter != predBasicBlocks.end()) {
        *iter = newPred;
    }
    replacePhiSource(oldPred, newPred);
}

// NOLINTEND