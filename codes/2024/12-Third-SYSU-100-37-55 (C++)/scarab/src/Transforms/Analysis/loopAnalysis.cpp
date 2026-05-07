#include "loopAnalysis.h"

static int loopCnt = 0;

void mapSubloopsToParentLoop(LoopPtr loop, FunctionPtr func, stack<BasicBlockPtr>& backedgeStack) {
    while (!backedgeStack.empty()) {
        auto pred = backedgeStack.top();
        backedgeStack.pop();
        auto subLoop = func->getLoopOfBB(pred);

        if (!subLoop) {
            func->bbToLoopMap[pred] = loop;
            if (pred != loop->header) {
                for (auto predecessorBB : pred->predBasicBlocks) {
                    backedgeStack.push(predecessorBB );
                }
            }
        }
        else {
            while (auto parent = subLoop->parentLoop) {
                subLoop = parent;
            }
            if (subLoop != loop) {
                subLoop->parentLoop = loop;
                auto header = subLoop->header;
                for (auto predecessorBB : header->predBasicBlocks) {
                    auto predLoop = func->getLoopOfBB(predecessorBB );
                    if (!predLoop || predLoop != subLoop) {
                        backedgeStack.push(predecessorBB );
                    }
                }
            }
        }
    }
}

int getLoopDepth(BasicBlockPtr bb) {
    auto func = bb->parentFunction;
    if(auto loop = func->getLoopOfBB(bb))
        return loop->loopDepth;
    return 0;
}

void computeExitBlocks(FunctionPtr func) {
    for (auto loop : func->loopList) {
        for (auto bb : loop->basicBlocks) {
            auto lastInstr = bb->instructions[bb->instructions.size()-1];
            if(lastInstr->type != Br) continue;
            auto br = dynamic_cast<BrInstruction *>(lastInstr.get());
            if (!br->exp) continue;
            auto bb1 = func->LabelToBBMap[br->label1];
            auto bb2 = func->LabelToBBMap[br->label2];
            if (!loop->contains(bb1) || !loop->contains(bb2)) {
                loop->exitingBlocks.insert(bb);
            }

            for (auto succBB : bb->succBasicBlocks) {
                if (!loop->contains(succBB)) {
                    loop->exitBlocks.insert(succBB);
                }
            }
        }
    }
}

void computePreHeader(FunctionPtr func) {
    for (auto loop : func->loopList) {
        BasicBlockPtr preHeader = nullptr;
        for (auto pred : loop->header->predBasicBlocks) {
            if (getLoopDepth(pred) != loop->loopDepth) {
                preHeader = pred;
                break;
            }
        }
        assert(preHeader != nullptr && preHeader->succBasicBlocks.size() == 1);
        loop->preHeader = preHeader;
        preHeader->isPreHeader = true;
    }
}

void computeLatch(FunctionPtr func) {
    for (auto loop : func->loopList) {
        BasicBlockPtr latch = nullptr;
        for (auto pred : loop->header->predBasicBlocks) {
            if (loop->contains(pred)) {
                latch = pred;
                break;
            }
        }
        loop->latchBlock = latch;
    }
}

PhiInstruction* getFinalPhi(Instruction* inst, LoopPtr loop){
    if(inst->type == Phi){
        auto phiInst = dynamic_cast<PhiInstruction*>(inst);
        return phiInst;
    }

    if(inst->type == Binary){
        auto binaryInst = dynamic_cast<BinaryInstruction*>(inst);
        auto lhs = binaryInst->a;
        auto rhs = binaryInst->b;

        if (auto defInstr = lhs->I; defInstr && !loop->isLoopInvariant(lhs))
            if (auto phi = getFinalPhi(defInstr, loop))
                return phi;

        if (auto defInstr = rhs->I; defInstr && !loop->isLoopInvariant(rhs))
            if (auto phi = getFinalPhi(defInstr, loop))
                return phi;
    }
    return nullptr;
}

void computeLoopIndVar(FunctionPtr func) {
    for (auto loop : func->loopList) {
        auto header = loop->header;
        auto lastInstr = header->instructions[header->instructions.size()-1];
        if(lastInstr->type != Br) continue;
        auto br = dynamic_cast<BrInstruction*>(lastInstr.get());
        if (!br->exp) continue;

        auto icmp = dynamic_cast<IcmpInstruction*>(br->exp->I);
        if (!icmp) {
            continue;
        }

        loop->inductionCondition = icmp->getSharedThis();
        loop->icmpKind = icmp->kind;

        auto icmpLhs = icmp->a;
        auto icmpRhs = icmp->b;

        if (loop->isLoopInvariant(icmpLhs)) {
            loop->inductionEnd = icmpLhs;
            if (auto instr = icmpRhs->I; instr && !loop->isLoopInvariant(icmpRhs)) {
                if (auto phi = getFinalPhi(instr, loop))
                    loop->inductionPhi = phi;
            }
        }
        else if (loop->isLoopInvariant(icmpRhs)) {
            loop->inductionEnd = icmpRhs;
            if (auto instr = icmpLhs->I; instr && !loop->isLoopInvariant(icmpLhs)) {
                if (auto phi = getFinalPhi(instr, loop))
                    loop->inductionPhi = phi;
            }
        }
    }
}

void computeLoopBB(FunctionPtr func) {
    for (auto bb : func->basicBlocks) {
        bb->loop = func->getLoopOfBB(bb);
        if (bb->loop) {
            bb->loopDepth = bb->loop->loopDepth;
        }
    }
}

void traverseDomTreePostOrder(BasicBlockPtr root, set<BasicBlockPtr> &visited, vector<BasicBlockPtr> &postOrderList){
    if(visited.count(root)) return;
    visited.insert(root);
    for (const auto& dominated : root->dominatedBlocks) {
        if(visited.count(dominated)) continue;
        traverseDomTreePostOrder(dominated, visited, postOrderList);
    }
    postOrderList.push_back(root);
}

void traversePostOrder(BasicBlockPtr root, set<BasicBlockPtr> &visited, FunctionPtr func){
    if(visited.count(root)) return;
    visited.insert(root);
    for (const auto& succ : root->succBasicBlocks) {
        traversePostOrder(succ, visited, func);
    }
    auto subLoop = func->getLoopOfBB(root);
    if (subLoop && root == subLoop->header) {
        if (subLoop->parentLoop)
            subLoop->parentLoop->subLoops.push_back(subLoop);
        else {
            func->outerLoops.push_back(subLoop);
            subLoop->loopDepth = 1;
        }
        reverse(subLoop->basicBlocks.begin() + 1, subLoop->basicBlocks.end());
        reverse(subLoop->subLoops.begin(), subLoop->subLoops.end());
        subLoop = subLoop->parentLoop;
    }

    while (subLoop) {
        subLoop->addBasicBlock(root);
        subLoop = subLoop->parentLoop;
    }
}

void loopAnalysis(FunctionPtr func) {
    func->clearBBToLoopMap();
    func->clearLoopList();

    vector<BasicBlockPtr> postOrderList;
    set<BasicBlockPtr> visited;

    traverseDomTreePostOrder(func->getEntryBlock(), visited, postOrderList);

    stack<BasicBlockPtr> backedgeStack;

    for (auto header : postOrderList) {
        for (auto pred : header->predBasicBlocks)
            if (pred->allDominators.count(header)) {
                backedgeStack.push(pred);
            }
        
        if (!backedgeStack.empty()) {
            auto loop = LoopPtr(new Loop(header, loopCnt++));
            mapSubloopsToParentLoop(loop, func, backedgeStack);
        }
    }

    visited.clear();
    
    traversePostOrder(func->getEntryBlock(), visited, func);

    stack<LoopPtr, vector<LoopPtr>> loopStack(func->outerLoops);
    while (!loopStack.empty()) {
        auto loop = loopStack.top();
        loopStack.pop();
        func->loopList.push_back(loop);
        for (auto subLoop : loop->subLoops) {
            subLoop->loopDepth = loop->loopDepth + 1;
            loopStack.push(subLoop);
        }
    }

    computeExitBlocks(func);
    computeLatch(func);
    computePreHeader(func);
    computeLoopIndVar(func);
    computeLoopBB(func);

    for (auto loop : func->outerLoops) {
        ScalarEvolution(loop);
    }
}