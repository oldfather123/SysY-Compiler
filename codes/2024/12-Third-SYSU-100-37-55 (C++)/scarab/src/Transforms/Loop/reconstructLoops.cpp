#include "reconstructLoops.h"

bool isLegalLoop(LoopPtr loop) {
    if (loop->exitingBlocks.size() != 1 || 
        loop->exitBlocks.size() != 1 || 
        !loop->subLoops.empty() || 
        loop->iterationCount != 0) {
        return false;
    }

    for (const auto& basicBlock : loop->basicBlockSet) {
        bool isHeader = (basicBlock == loop->header);
        for (const auto& instr : basicBlock->instructions) {
            if (instr->type == Call || (!isHeader && instr->type == Icmp)) {
                return false;
            }
        }
    }

    auto inductionPhi = loop->inductionPhi;
    if (!loop->hasSCEV(inductionPhi)) {
        return false;
    }

    const auto& scevExpression = loop->getSCEV(inductionPhi);
    auto initialValue = scevExpression.at(0);
    auto stepValue = scevExpression.at(1);
    auto endValue = loop->inductionEnd;

    if (initialValue && initialValue->isConst && stepValue && stepValue->isConst && endValue) {
        auto initialConstant = dynamic_cast<Const *>(initialValue.get());
        auto stepConstant = dynamic_cast<Const *>(stepValue.get());
        if (initialConstant && stepConstant && initialConstant->intVal == 0 && stepConstant->intVal == 1) {
            return true;
        }
    }

    return false;
}

void reconstructLoop(LoopPtr loop, FunctionPtr func) {
    if (!isLegalLoop(loop)) {
        return;
    }

    auto exitBlock = *loop->exitBlocks.begin();
    auto remainderInstruction = exitBlock->instructions[0];
    auto remainderBinaryInstr = dynamic_cast<BinaryInstruction*>(remainderInstruction.get());

    if (!remainderBinaryInstr || remainderBinaryInstr->op != '%') {
        return;
    }

    InstructionPtr targetInstruction = nullptr;
    for (const auto& instr : loop->latchBlock->instructions) {
        auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
        if (binaryInstr && (binaryInstr->a == remainderBinaryInstr->a || binaryInstr->b == remainderBinaryInstr->b)) {
            targetInstruction = instr;
            break;
        }
    }

    if (!targetInstruction) return;

    auto targetBinaryInstr = dynamic_cast<BinaryInstruction*>(targetInstruction.get());
    if (!targetBinaryInstr) {
        return;
    }

    if (targetBinaryInstr->op == '+' && !targetBinaryInstr->a->isConst && targetBinaryInstr->b->isConst) {
        auto indexValue = loop->inductionEnd;
        auto newInstruction = InstructionPtr(new BinaryInstruction(indexValue, targetBinaryInstr->b, '*', BasicBlockPtr(nullptr)));

        auto newBinaryInstr = dynamic_cast<BinaryInstruction*>(newInstruction.get());
        if (newBinaryInstr) {
            newBinaryInstr->isrv64 = true;
        }
        remainderBinaryInstr->isrv64 = true;

        exitBlock->insertInstructionBefore(newInstruction, remainderInstruction);
        replaceVarByVar(remainderBinaryInstr->a, newInstruction->reg);
        deleteUser(remainderBinaryInstr->a);

        for (const auto& instr : loop->preHeader->instructions) {
            if (instr->type == Br) {
                auto branchInstr = dynamic_cast<BrInstruction*>(instr.get());
                if (branchInstr && !branchInstr->label2) {
                    branchInstr->label1 = exitBlock->label;
                }
            }
        }

        exitBlock->predBasicBlocks.clear();
        exitBlock->predBasicBlocks.insert(loop->preHeader);
    }
}

void reconstructLoops(FunctionPtr func) {
    queue<LoopPtr> workQueue;
    for (const auto& loop : func->loopList) {
        if (!loop->parentLoop) {
            workQueue.push(loop);
        }
    }

    while (!workQueue.empty()) {
        auto currentLoop = workQueue.front();
        workQueue.pop();
        reconstructLoop(currentLoop, func);
        for (const auto& subLoop : currentLoop->subLoops) {
            workQueue.push(subLoop);
        }
    }
}