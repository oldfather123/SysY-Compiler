#include "SCEV.h"

void convertBIVToSCEV(LoopPtr loop, PhiInstruction* phiInstr, set<ValuePtr>& addOperands, set<ValuePtr>& subOperands) {
    ValuePtr initialValue = nullptr;
    for (auto entry : phiInstr->from) {   
        auto candidateValue = entry.first;
        if (loop->isLoopInvariant(candidateValue)) {
            initialValue = candidateValue;
            break;
        }
    }
    if (!initialValue) {
        return;
    }

    set<BinaryInstruction*> generatedInstructions;
    ValuePtr stepValue = nullptr;

    auto createBinaryOperation = [&](ValuePtr lhsValue, ValuePtr rhsValue, char op) -> ValuePtr {
        auto binaryInstr = new BinaryInstruction(lhsValue, rhsValue, op, BasicBlockPtr(nullptr));
        generatedInstructions.insert(binaryInstr);
        return binaryInstr->reg;
    };

    if (!addOperands.empty()) {
        stepValue = *addOperands.begin();
        addOperands.erase(stepValue);
        for (auto& sub : subOperands) {
            stepValue = createBinaryOperation(stepValue, sub, '-');
        }
        for (auto& add : addOperands) {
            stepValue = createBinaryOperation(stepValue, add, '+');
        }
    }
    else if (!subOperands.empty()) {
        stepValue = *subOperands.begin();
        subOperands.erase(stepValue);
        int zeroValue = 0;
        for (auto& sub : subOperands) {
            stepValue = createBinaryOperation(ValuePtr(new Const(zeroValue, "scevSubTemp")), sub, '-');
        }
    }

    if (stepValue) {
        SCEV scev(initialValue, stepValue, std::move(generatedInstructions));
        loop->registerSCEV((Instruction*)phiInstr, scev);
    }
}

void registerBIV(LoopPtr loop, PhiInstruction* phiInstr) {
    stack<BinaryInstruction *> binaryInstructionStack;
    set<ValuePtr> addOperands;
    set<ValuePtr> subOperands;
    for (auto& use : phiInstr->from) {
        auto instr = dynamic_cast<BinaryInstruction*>(use.first->I);
        if (instr && !use.first->isConst && instr->basicblock) {
            binaryInstructionStack.push(instr);
        }
    }

    while (!binaryInstructionStack.empty()) {
        auto binaryInstr = binaryInstructionStack.top();
        binaryInstructionStack.pop();

        if (binaryInstr->op == '+') {
            if (loop->isLoopInvariant(binaryInstr->a))
                addOperands.insert(binaryInstr->a);
            else if (loop->isLoopInvariant(binaryInstr->b))
                addOperands.insert(binaryInstr->b);
            else
                return;
        }
        else if (binaryInstr->op == '-') {
            if (loop->isLoopInvariant(binaryInstr->b))
                subOperands.insert(binaryInstr->b);
            else
                return;
        }
        for (auto usePtr = binaryInstr->reg->useHead; usePtr != nullptr; usePtr = usePtr->next) {
            auto userInstr = usePtr->user;
            if (userInstr->type == Phi && dynamic_cast<PhiInstruction*>(userInstr)->reg == phiInstr->reg) {
                convertBIVToSCEV(loop, phiInstr, addOperands, subOperands);
                return;
            } 
            else if (userInstr->type == Binary) {
                auto binaryInstr = dynamic_cast<BinaryInstruction*>(userInstr);
                if (binaryInstr && (binaryInstr->op == '+' || binaryInstr->op == '-')) {
                    binaryInstructionStack.push(binaryInstr);
                }
            }
        }
    }
}

void identifyAndRegisterBIVs(LoopPtr loop) {
    vector<PhiInstruction*> phiInstructions;
    for (auto& instr : loop->header->instructions) {
        if (instr->type == Phi) {
            auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
            phiInstructions.push_back(phiInstr);
        }   
        else {
            break;
        }
    }

    for (auto phiInstr : phiInstructions) {
        stack<Instruction*> instructionStack;
        for(auto& use : phiInstr->from) {
            for (auto usePtr = use.first->useHead; usePtr != nullptr; usePtr = usePtr->next) {
                instructionStack.push(usePtr->user);
            }
        }

        while (!instructionStack.empty()) {
            auto instr = instructionStack.top();
            instructionStack.pop();
            if(instr == phiInstr) {
                registerBIV(loop, phiInstr);
                break;
            }

            if (auto phiInstr = dynamic_cast<PhiInstruction*>(instr)) {
                break;
            }

            auto regValue = instr->reg;
            if (regValue) {
                auto usePtr = regValue->useHead;
                while (usePtr != nullptr) {
                    auto userInstr = usePtr->user;
                    assert(userInstr);
                    instructionStack.push(userInstr);
                    usePtr = usePtr->next;
                }
            }
        }
    }
}

void linkBinaryInstrToSCEV(LoopPtr loop, BinaryInstruction* binaryInstr, bool &hasChanged) {
    auto lhsValue = binaryInstr->a;
    auto rhsValue = binaryInstr->b;
    auto lhsInstr = lhsValue-> I;
    auto rhsInstr = rhsValue-> I;

    auto registerAndMarkChanged = [&](auto scev) {
        loop->registerSCEV(binaryInstr, scev);
        hasChanged = false;
    };

    if (loop->hasSCEV(lhsInstr)) {
        auto lhsSCEV = loop->getSCEV(lhsInstr);
        if (loop->isLoopInvariant(rhsValue)) {
            switch (binaryInstr->op) {
                case '+': registerAndMarkChanged(rhsValue + lhsSCEV); return;
                case '*': registerAndMarkChanged(rhsValue * lhsSCEV); return;
                case '-': registerAndMarkChanged(lhsSCEV - rhsValue); return;
            }
        }
        else if (loop->hasSCEV(rhsInstr)) {
            auto rhsSCEV = loop->getSCEV(rhsInstr);
            switch (binaryInstr->op) {
                case '+': registerAndMarkChanged(lhsSCEV + rhsSCEV); return;
                case '-': registerAndMarkChanged(lhsSCEV - rhsSCEV); return;
            }
        }
    }

    if (loop->hasSCEV(rhsInstr)) {
        auto rhsSCEV = loop->getSCEV(rhsInstr);
        if (loop->isLoopInvariant(lhsValue)) {
            switch (binaryInstr->op) {
                case '+': registerAndMarkChanged(lhsValue + rhsSCEV); return;
                case '*': registerAndMarkChanged(lhsValue * rhsSCEV); return;
            }
        }
    }
}

void ScalarEvolution(LoopPtr loop) {
    if(!loop) return;
    
    for (const auto& subLoop : loop->subLoops) {
        ScalarEvolution(subLoop);
    }
    loop->scevMap.clear();
    identifyAndRegisterBIVs(loop);
    bool hasChanged;
    do {
        hasChanged = true;
        for(auto& basicBlock : loop->basicBlockSet) {
            for(auto& instr : basicBlock->instructions) {
                if(!loop->hasSCEV(instr.get()) && instr->type == Binary) {
                    auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
                    if(binaryInstr->op=='+' || binaryInstr->op =='-' || binaryInstr->op=='*') {
                        linkBinaryInstrToSCEV(loop, binaryInstr, hasChanged);
                    }
                }
            }
        }
    } while(!hasChanged);

    if(loop->inductionPhi && loop->hasSCEV(loop->inductionPhi)) {
        auto& scev = loop->getSCEV(loop->inductionPhi);
        auto endValue = loop->inductionEnd;
        auto initialValue = scev.at(0);
        auto stepValue = scev.at(1);
        if(endValue && endValue->isConst && initialValue && initialValue ->isConst && stepValue && stepValue->isConst) {
            auto constEndValue = dynamic_cast<Const *>(endValue.get());
            auto constInitialValue = dynamic_cast<Const *>(initialValue.get());
            auto constStepValue = dynamic_cast<Const *>(stepValue.get());
            if (constEndValue && constInitialValue && constStepValue) {
                int iterationCount = 0;
                switch (loop->icmpKind) {
                    case ICmpEQ:
                    case ICmpSGE:
                    case ICmpSLE:
                        iterationCount = (constEndValue->intVal + 1 - constInitialValue->intVal) / constStepValue->intVal;
                        break;
                    default:
                        iterationCount = (constEndValue->intVal - constInitialValue->intVal) / constStepValue->intVal;
                        break;
                }
                loop->iterationCount = iterationCount;
            }
        }
    }
}