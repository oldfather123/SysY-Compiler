#include "LICM.h"

bool isArgumentIdempotent(ValuePtr arg) {
    if(auto var = dynamic_cast<Variable *>(arg.get())) {
        if(var->isGlobal || var->type->getID() == PtrID || (var->I && var->I->type == GEP))
            return false;
    }
    else if(auto constant = dynamic_cast<Const *>(arg.get())) {
        if(constant->type->getID() == PtrID)
            return false;
    }
    return true;
}

bool isFunctionIdempotent(FunctionPtr func, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    if(func->isLibraryFunction)
        return false;

    for(auto &basicBlock: func->basicBlocks) {
        for(auto &instr: basicBlock->instructions) {
            switch(instr->type) {
                case Load: {
                    auto loadInstr = dynamic_cast<LoadInstruction *>(instr.get());
                    if(dynamic_cast<Variable *>(loadInstr->from.get()) || loadInstr->gep)
                        return false;
                    break;
                }
                case Store: {
                    auto storeInstr = dynamic_cast<StoreInstruction *>(instr.get());
                    if(dynamic_cast<Variable *>(storeInstr->des.get()) || storeInstr->gep)
                        return false;
                    break;
                }
                case GEP:
                    return false;
                case Call: {
                    auto callInstr = dynamic_cast<CallInstruction *>(instr.get());
                    if(idempotentFunctions.find(callInstr->func) != idempotentFunctions.end()) {
                        if(!idempotentFunctions[callInstr->func])
                            return false;
                    }
                    else {
                        if(!isCallIdempotent(callInstr, idempotentFunctions))
                            return false;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    return true;
}

bool isCallIdempotent(CallInstruction *instr, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    for(auto &arg : instr->argv) {
        if(!isArgumentIdempotent(arg)) {
            return false;
        }
    }

    auto func = instr->func;
    if(idempotentFunctions.find(func) != idempotentFunctions.end())
        return idempotentFunctions[func];

    idempotentFunctions[func] = false;
    bool isIdempotent = isFunctionIdempotent(func, idempotentFunctions);
    idempotentFunctions[func] = isIdempotent;
    func->isReentrant = isIdempotent;

    return isIdempotent;
}

bool isSafeToSpeculativelyExecute(InstructionPtr instr, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    if (instr->type == Binary) {
        auto binaryInstr = dynamic_cast<BinaryInstruction *>(instr.get());
        if((binaryInstr->op == '/' || binaryInstr->op == '%') && dynamic_cast<Const *>(binaryInstr->b.get())) {
            auto constantValue = dynamic_cast<Const *>(binaryInstr->b.get());
            return constantValue->type->getID() != IntID || constantValue->intVal != 0;
        }
    }
    else if(instr->type == Call) {
        return isCallIdempotent(dynamic_cast<CallInstruction *>(instr.get()), idempotentFunctions);
    } 
    return !(instr->type == Alloca || instr->type == Phi || instr->type == Return || instr->type == Br || instr->type == Store);
}

bool isSafeToHoist(InstructionPtr instr, LoopPtr currentLoop, FunctionPtr func, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    if (instr->type == Call || isSafeToSpeculativelyExecute(instr, idempotentFunctions)) {
        return true;
    }

    auto instructionBlock = instr->basicblock;
    auto entryBlock = func->basicBlocks.front();
    for (const auto& exitBlock : currentLoop->exitBlocks) {
        if (!Loop::dominates(instructionBlock, exitBlock, entryBlock)) {
            return false;
        }
    }
    return true;
}

bool isArgumentInCall(ValuePtr value, CallInstruction *callInstr) {
    for (const auto& arg : callInstr->argv) {
        if (arg == value) {
            return true;
        }

        if (auto argGEP = dynamic_cast<GetElementPtrInstruction *>(arg->I)) {
            if (argGEP->from == value) {
                return true;
            }
        }
    }
    return false;
}

bool isModifiedInLoop(LoadInstruction *loadInstr, LoopPtr currentLoop) {
    auto source = loadInstr->from;

    while (auto gepInstr = dynamic_cast<GetElementPtrInstruction *>(source->I)) {
        source = gepInstr->from;
    }

    for (auto basicBlock : currentLoop->basicBlocks) {
        for (auto loopInstr : basicBlock->instructions) {
            if (loopInstr->type == Store) {
                auto storeInstr = dynamic_cast<StoreInstruction *>(loopInstr.get());
                auto destination = storeInstr->des;

                if (source == destination)
                    return true;

                while (auto destinationGEP = dynamic_cast<GetElementPtrInstruction *>(destination->I)) {
                    if (destinationGEP->from == source)
                        return true;
                    destination = destinationGEP->from;
                }
            } 
            else if (loopInstr->type == Call) {
                auto callInstr = dynamic_cast<CallInstruction *>(loopInstr.get());

                if (isArgumentInCall(source, callInstr))
                    return true;

                auto sourceVariable = dynamic_cast<Variable *>(source.get());
                if (sourceVariable && sourceVariable->isGlobal) {
                    if (!callInstr->func->isLibraryFunction) {
                        for (const auto& calledBB : callInstr->func->basicBlocks) {
                            for (const auto& calledInstr : calledBB->instructions) {
                                if (calledInstr->type == Store) {
                                    auto storeInstr = dynamic_cast<StoreInstruction *>(calledInstr.get());
                                    auto destination = storeInstr->des;

                                    if (source == destination)
                                        return true;

                                    while (auto destinationGEP = dynamic_cast<GetElementPtrInstruction *>(destination->I)) {
                                        if (destinationGEP->from == source)
                                            return true;
                                        destination = destinationGEP->from;
                                    }
                                }
                                else if (calledInstr->type == Call) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool isLoopInvariant(InstructionPtr instr, LoopPtr currentLoop, FunctionPtr func, unordered_set<Instruction *>& toDelete, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    auto type = instr->type;
    
    if (type == Phi) {
        return false;
    }
    
    bool condition1 = (type == Alloca
        || (type == Load && !isModifiedInLoop(dynamic_cast<LoadInstruction *>(instr.get()), currentLoop))
        || (type == Call && isCallIdempotent(dynamic_cast<CallInstruction *>(instr.get()), idempotentFunctions))
        || type == Bitcast || type == Sitofp || type == Fptosi
        || type == GEP || type == Binary || type == Fneg || type == Ext);
    
    bool condition2 = true;
    int operandCount = instr->getOperandCount();
    for (int i = 0; i < operandCount; i++) {
        auto operand = instr->getOperand(i);
        bool isConst = operand->isConst;
        bool isComputedOutsideLoop = (operand->I == nullptr 
            || !currentLoop->contains(operand->I->basicblock)
            || toDelete.find(operand->I) != toDelete.end());

        if (!(isConst || isComputedOutsideLoop)) {
            condition2 = false;
            break;
        }
    }

    return condition1 && condition2;
}

bool canMoveToExit(InstructionPtr instr, LoopPtr currentLoop, vector<InstructionPtr> &toMoveExitInst) {
    if(instr->type != Binary) return false;

    auto binaryInstr = dynamic_cast<BinaryInstruction *>(instr.get());
    if(binaryInstr->op != '%' || !binaryInstr->b->isConst) return false;

    auto constant = dynamic_cast<Const *>(binaryInstr->b.get());
    assert(constant->type->isInt());

    Value *currentValue = binaryInstr->reg.get();
    auto useHead = currentValue->useHead;
    int userInLoop = 0;
    PhiInstruction *phiInstr = nullptr;
    
    while(useHead) {
        if(useHead->user->basicblock->loop == currentLoop) {
            currentValue = useHead->userValue;
            userInLoop++;
        }
        useHead = useHead->next;
    }

    if(userInLoop != 1 || !(phiInstr = dynamic_cast<PhiInstruction *>(currentValue->I))) {
        return false;
    }

    useHead = phiInstr->reg->useHead;
    userInLoop = 0;
    while(useHead) {
        if(useHead->user->basicblock->loop == currentLoop) {
            currentValue = useHead->userValue;
            userInLoop++;
        }
        useHead = useHead->next;
    }

    if(userInLoop == 1 && currentValue == binaryInstr->a.get()) {
        auto userInstr = dynamic_cast<BinaryInstruction *>(currentValue->I);
        if(userInstr && (userInstr->op == '+' || userInstr->op == '-')) {
            replaceVarByVar(binaryInstr->reg, binaryInstr->a);
            binaryInstr->setOperands(0, phiInstr->reg);

            Use *use = phiInstr->reg->useHead;
            while (use) {
                auto user = use->user;
                auto nextUse = use->next;
                if (user->basicblock->loop != currentLoop) {
                    user->replaceOperand(phiInstr->reg, binaryInstr->reg);
                    use->rmUse();
                    newUse(binaryInstr->reg.get(), user);
                }
                use = nextUse;
            }
            return true;
        }
    }

    return false;
}

void processLoop(LoopPtr currentLoop, FunctionPtr func, bool licmHasModifications, unordered_map<FunctionPtr, bool>& idempotentFunctions) {
    auto preheader = currentLoop->preHeader;
    queue<BasicBlockPtr> basicBlockQueue;
    unordered_set<BasicBlockPtr> visitedBlocks;
    visitedBlocks.insert(preheader);

    for(const auto& childBlock: preheader->dominatedBlocks) {
        if(currentLoop->contains(childBlock) && childBlock->loop == currentLoop) {
            basicBlockQueue.push(childBlock);
        }
    }

    while(!basicBlockQueue.empty()) {
        BasicBlockPtr basicBlock = basicBlockQueue.front();
        basicBlockQueue.pop();
        if(visitedBlocks.find(basicBlock) != visitedBlocks.end()) continue;
        visitedBlocks.insert(basicBlock);

        for(const auto& childBlock: basicBlock->dominatedBlocks) {
            if(currentLoop->contains(childBlock) && childBlock->loop == currentLoop) {
                basicBlockQueue.push(childBlock);
            }
        }

        unordered_set<Instruction *> instructionsToDelete;
        vector<InstructionPtr> instructionsToHoist;
        vector<InstructionPtr> instructionsToMoveToExit;

        for(auto& instr : basicBlock->instructions) {
            if(licmHasModifications && canMoveToExit(instr, currentLoop, instructionsToMoveToExit)) {
                instructionsToMoveToExit.push_back(instr);
                instructionsToDelete.insert(instr.get());
                continue;
            }

            if(isLoopInvariant(instr, currentLoop, func, instructionsToDelete, idempotentFunctions) && isSafeToHoist(instr, currentLoop, func, idempotentFunctions)) {
                instructionsToHoist.push_back(instr);
                instructionsToDelete.insert(instr.get());
            }
        }

        if(!instructionsToDelete.empty()) {
            preheader->appendInstruction(instructionsToHoist);
            basicBlock->instructions.erase(
                remove_if(basicBlock->instructions.begin(), basicBlock->instructions.end(),
                          [&instructionsToDelete](InstructionPtr instr) {
                              return instructionsToDelete.count(instr.get()) > 0;
                          }),
                basicBlock->instructions.end()
            );
        }

        if(licmHasModifications && !instructionsToMoveToExit.empty()) {
            for(auto exitBlock: currentLoop->exitBlocks) {
                exitBlock->prependInstructions(instructionsToMoveToExit);
            }
        }
    }
}

void LICM(FunctionPtr func, bool hasModifications) {
    unordered_map<FunctionPtr, bool> idempotentFunctions;
    vector<LoopPtr> loopsInPostorder = func->getLoopsInPostorder();
    for (auto& loop : loopsInPostorder) {
        processLoop(loop, func, hasModifications, idempotentFunctions);
    }
}