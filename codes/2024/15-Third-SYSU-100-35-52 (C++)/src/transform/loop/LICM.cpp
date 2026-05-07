#include "Block.h"
#include "CFGAnalysis.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "LICM.h"
#include "Type.h"
#include "domTreeAnalysis.h"
#include "simplifyCFG.h"
#include <iostream>

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/Loop/LICMMemory.cpp#L4

// a != b , 但是a b 之间可能有别名关系, 指向同一个地址
// TODO: do more 区分 
static bool isDistinctd(Value* a, Value* b) {

    if(a->is<GetElementPtrInstruction>() && b->is<GetElementPtrInstruction>()) {
        auto gep1 = a->as<GetElementPtrInstruction>();
        auto gep2 = b->as<GetElementPtrInstruction>();
        if(gep1->getBase() == gep2->getBase()) {
            if(gep1->getIdx().size() != gep2->getIdx().size()) {
                return true;
            }
            for(int i = 0; i < gep1->getIdx().size(); i++) {
                auto const1 = dynamic_cast<Const*>(gep1->getIdx()[i]);
                auto const2 = dynamic_cast<Const*>(gep2->getIdx()[i]);
                if(const1 == nullptr || const2 == nullptr) {
                    return false;
                }
                if(const1->intVal != const2->intVal) {  // NOLINT
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

static bool runOnLoop(BasicBlock* block, const CFGAnalysisResult& cfg) {

    IRbuilder builder{ block };

    std::unordered_map<Value*, std::vector<Instruction*>> memoryInst;

    for(auto& inst : block->instructionsRef()) {
        // use my sysu-lang2 form
        if(inst->insId == InsID::Call)  // TODO: allow no-side-effect call
            return false;

        if(inst->insId == InsID::Store || inst->insId == InsID::Load || inst->insId == InsID::AtomicAdd) {
            auto addr = inst->getOperand(0);
            if(addr->getBasicBlock() != block)  // static address memory instruction
                memoryInst[addr].push_back(inst.get());
        }
    }

    bool modified = false;
    BasicBlock* preBody = nullptr;
    BasicBlock* postBody = nullptr;

    auto preBodyOrCreate = [&]() mutable {
        if(!preBody) {
            modified = true;
            preBody = builder.addBasicBlock("prebody");
        }
        return preBody;
    };

    auto postBodyOrCreate = [&]() mutable {
        if(!postBody) {
            modified = true;
            postBody = builder.addBasicBlock("postbody");
        }
        return postBody;
    };

    vector<Instruction*> toRemove;
    for(auto& [addr, insts] : memoryInst) {
        bool isDistinct = true;
        for(auto& inst : block->instructionsRef())
            if(inst->insId == InsID::Store || inst->insId == InsID::Load || inst->insId == InsID::AtomicAdd) {
                auto thatAddr = inst->getOperand(0);
                if(thatAddr != addr && !isDistinctd(addr, thatAddr) ) {
                    isDistinct = false;
                    break;
                }
            }

        if(!isDistinct)
            continue;

        bool hasAtomic =
            std::any_of(insts.begin(), insts.end(), [](Instruction* inst) { return inst->insId == InsID::AtomicAdd; });
        if(hasAtomic)
            continue;
        bool hasLoad = std::any_of(insts.begin(), insts.end(), [](Instruction* inst) { return inst->insId == InsID::Load; });
        bool hasStore = std::any_of(insts.begin(), insts.end(), [](Instruction* inst) { return inst->insId == InsID::Store; });

        Type* phiType = addr->getType();
        //while (phiType->is<PtrType>()) {
        //    phiType = phiType->as<PtrType>()->inner;
        //}
        auto phiInst = new PhiInstruction(phiType);  // NOLINT
        // phiInst->insertBefore(block, block->instructions().begin());
        insertBefore(block, block->instructionsRef().begin(), phiInst);
        phiInst = block->instructionsRef().front().get()->as<PhiInstruction>();

        if(hasLoad) {
            BasicBlock* pre = preBodyOrCreate();
            Instruction* loadInst = new LoadInstruction(addr, phiType);
            loadInst->setLabel("load");
            // loadInst->insertBefore(pre, pre->instructions().end());
            insertBefore(pre, pre->instructionsRef().end(), loadInst);
            loadInst = pre->instructionsRef().back().get();
            phiInst->addIncoming(pre, loadInst);
        }

        Value* currentValue = phiInst;

        for(auto& inst : insts) {
            if(inst->insId == InsID::Store) {
                currentValue = inst->getOperand(1);
                toRemove.push_back(inst);
            } else {
                inst->replaceWith(currentValue);
                toRemove.push_back(inst);
            }
        }

        phiInst->addIncoming(block, currentValue);

        if(hasStore) {
            BasicBlock* post = postBodyOrCreate();
            // Instruction* storeInst = make<StoreInst>(addr, currentValue);
            // storeInst->insertBefore(post, post->instructions().end());
            auto storeInst = new StoreInstruction(addr, currentValue);  // NOLINT
            insertBefore(post, post->instructionsRef().end(), storeInst);
        }
    }

    for(auto inst : toRemove)
        inst->eraseFromParent();

    if(preBody) {
        for(auto& inst : block->instructionsRef())
            if(inst->insId == InsID::Phi) {
                auto thisPhi = inst.get()->as<PhiInstruction>();  // NOLINT
                if(std::any_of(thisPhi->incomings().begin(), thisPhi->incomings().end(),
                               [&](auto& p) { return p.first == preBody; }))
                    continue;

                auto phi = inst->clone()->as<PhiInstruction>();  // pre-body's phi
                phi->removeSource(block);
                // phi->insertBefore(preBody, preBody->instructions().begin());
                insertBefore(preBody, preBody->instructionsRef().begin(), phi);
                phi = preBody->instructionsRef().front().get()->as<PhiInstruction>();

                inst.get()->as<PhiInstruction>()->keepOneIncoming(block);     // NOLINT
                inst.get()->as<PhiInstruction>()->addIncoming(preBody, phi);  // NOLINT
            }

        for(auto& pred : cfg.predecessors(block)) {
            if(pred != block)
                resetTarget(pred->getTerminator()->as<BrInstruction>(), block, preBody);
        }
        // builder.setInsertPoint(preBody->instructions().end());
        builder.setInsertPoint(preBody, preBody->instructionsRef().end());
        builder.createJump(block);
        // builder.makeOp<BranchInst>(block);
    }

    if(postBody) {
        BasicBlock* exit = block->getTerminator()->as<BrInstruction>()->getFalseTarget();
        assert(exit != block);
        resetTarget(block->getTerminator()->as<BrInstruction>(), exit, postBody);
        retargetBlock(exit, block, postBody);
        builder.setInsertPoint(postBody, postBody->instructionsRef().end());
        // builder.makeOp<BranchInst>(exit);
        builder.createJump(exit);
    }

    return modified;
}

bool runLICM(FunctionPtr func, DominateAnalysisResult& dom, CFGAnalysisResult& cfg) {

    bool modified = false;
    std::unordered_set<BasicBlock*> visited;
    while(true) {
        auto loops = runLoopAnalysis(func, dom);
        bool changed = false;
        for(auto& loop : loops.loops) {
            if(loop.header == loop.latch) {
                if(!visited.insert(loop.latch).second)
                    continue;
                if(runOnLoop(loop.latch, cfg)) {
                    changed = true;
                    //func->dump(std::cout);
                    cfg = runCFGAnalysis(func);
                    dom = runDominateTreeAnalysis(func, cfg);
                    modified = true;
                    break;
                }
            }
        }
        if(!changed)
            break;
    }
    return modified;
}