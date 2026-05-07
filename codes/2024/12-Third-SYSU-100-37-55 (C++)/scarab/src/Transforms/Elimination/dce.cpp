#include "dce.h"


void markDeadCode(Instruction* instr, unordered_map<Instruction*, bool>& deletelist) {
    stack<Instruction*> worklist;
    worklist.push(instr);

    auto processOperand = [&](Instruction* currInstr, ValuePtr operand) {
        deletelist[currInstr] = true;
        rmInsUse(currInstr, operand);
        if (operand && operand->I && deletelist.find(operand->I) == deletelist.end()) {
            worklist.push(operand->I);
        }
    };

    unordered_set<InsID> validTypes = {
        Bitcast, Ext, Sitofp, Fptosi, Call, GEP, Binary, Fneg, Icmp, Fcmp, Phi, Load
    };

    while (!worklist.empty()) {
        auto currInstr = worklist.top();
        worklist.pop();
        if (!currInstr || validTypes.find(currInstr->type) == validTypes.end()) {
            continue;
        }

        if (currInstr->reg->useHead) {
            continue;
        }

        switch (currInstr->type) {
            case Bitcast: {
                auto castInstr = static_cast<BitCastInstruction*>(currInstr);
                    processOperand(currInstr, castInstr->from);
                break;
            }
            case Ext: {
                auto extInstr = static_cast<ExtInstruction*>(currInstr);
                processOperand(currInstr, extInstr->from);
                break;
            }
            case Sitofp: {
                auto sitofpInstr = static_cast<SignToFloatInstruction*>(currInstr);
                processOperand(currInstr, sitofpInstr->from);
                break;
            }
            case Fptosi: {
                auto fptosiInstr = static_cast<FloatToSignInstruction*>(currInstr);
                processOperand(currInstr, fptosiInstr->from);
                break;
            }
            case Call: {
                auto callInstr = static_cast<CallInstruction*>(currInstr);
                if (!callInstr->func->isLibraryFunction && callInstr->func->isReentrant) {
                    deletelist[currInstr] = true;
                    for (auto& arg : callInstr->argv) {
                        processOperand(currInstr, arg);
                    }
                }
                break;
            }
            case Binary: {
                auto binaryInstr = static_cast<BinaryInstruction*>(currInstr);
                    processOperand(currInstr, binaryInstr->a);
                    processOperand(currInstr, binaryInstr->b);
                break;
            }
            case Icmp: {
                auto icmpInstr = static_cast<IcmpInstruction*>(currInstr);
                    processOperand(currInstr, icmpInstr->a);
                    processOperand(currInstr, icmpInstr->b);
                break;
            }
            case Fcmp: {
                auto fcmpInstr = static_cast<FcmpInstruction*>(currInstr);
                    processOperand(currInstr, fcmpInstr->a);
                    processOperand(currInstr, fcmpInstr->b);
                break;
            }
            case Phi: {
                auto phiInstr = static_cast<PhiInstruction*>(currInstr);
                for (auto& from : phiInstr->from) {
                    processOperand(currInstr, from.first);
                }
                break;
            }
            case Load: {
                auto loadInstr = static_cast<LoadInstruction*>(currInstr);
                processOperand(currInstr, loadInstr->from);
                break;
            }
            default:
                break;
        } 
    }
    return;
}


void eliminateDeadCode(FunctionPtr func) {
    unordered_map<Instruction*, bool> deletelist;

    // 标记所有需要删除的指令
    for (auto& bb : func->basicBlocks) {
        for (auto& instr : bb->instructions) {
            markDeadCode(instr.get(), deletelist);
        }
    }

    // 移除标记为删除的指令
    for (auto& bb : func->basicBlocks) {
        bb->instructions.erase(
            std::remove_if(bb->instructions.begin(), bb->instructions.end(),
                [&](const InstructionPtr& instr) {
                    return deletelist.find(instr.get()) != deletelist.end();
                }),
            bb->instructions.end()
        );
    }
}


void eliminateDeadCodeInModule(Module& module) {
    bool hasChanges = false;

    // 构建调用图函数
    auto buildCallGraph = [&](Module& mod) {
        for (auto& func : mod.globalFunctions) {
            func->calleeMap.clear();
            func->callerMap.clear();
            func->callerInstructions.clear();
        }
        for (auto& func : mod.globalFunctions) {
            if (!func->isLibraryFunction) {
                func->calculateCallerAndCalleeInfo();
            }
        }
    };

    while(!hasChanges){
        hasChanges = true;
        buildCallGraph(module);
        
        for (auto& func: module.globalFunctions) {
            if (func->isLibraryFunction || func->returnValue->type->isVoid() || func->name == "main") {
                continue;
            }

            bool returnUsed = false;
            unordered_map<ValuePtr, PhiInstruction*> phiReturnMap;
            for (auto& instr: func->callerInstructions) {
                if (auto callInstr = dynamic_cast<CallInstruction*>(instr.get())) {
                    if (callInstr->reg->useHead) {
                        auto use = callInstr->reg->useHead;
                        if (callInstr->func == callInstr->basicblock->parentFunction) {
                            if (dynamic_cast<ReturnInstruction*>(use->user)) {
                                continue;
                            }
                            else if (auto phiInstr = dynamic_cast<PhiInstruction *>(use->user)) {
                                if (phiInstr->reg->useHead && phiInstr->reg->useCount == 1 && phiInstr->reg->useHead->user->type == Return) {
                                    phiReturnMap[callInstr->reg] = phiInstr;
                                    continue;
                                }
                            }
                        }
                        returnUsed = true;
                        break;
                    }
                }
            }

            if (!returnUsed) {
                bool functionChanged = false;
                for (auto& bb: func->basicBlocks) {
                    for (auto& instr: bb->instructions) {
                        if (auto returnInstr = dynamic_cast<ReturnInstruction *>(instr.get())) {
                            assert(returnInstr->retValue);
                            if (returnInstr->retValue->I) {
                                rmInsUse(returnInstr, returnInstr->retValue);
                                if (returnInstr->retValue->type->isInt()) {
                                    returnInstr->retValue = Const::createConstant(Type::getInt(), int(0), "pseudoReturnVal");
                                }
                                else if(returnInstr->retValue->type->isFloat()) {
                                    returnInstr->retValue = Const::createConstant(Type::getFloat(), float(0), "pseudoReturnVal");
                                }
                                else {
                                    assert(false && "unknow return type");
                                }
                                functionChanged = true;
                            }
                        }
                    }
                }
                for(auto& [reg, phiInstr] : phiReturnMap) {
                    for(auto iter = phiInstr->from.begin(); iter != phiInstr->from.end(); iter++) {
                        if(iter->first == reg) {
                            phiInstr->from.erase(iter);
                            break;
                        }
                    }
                    rmInsUse(phiInstr, reg);
                }
                if(functionChanged) {
                    eliminateDeadCode(func);
                    hasChanges = false;
                }
            }
        }
    }
}


bool isLoopDeadCodeEliminable(InstructionPtr instr) {
    // 如果指令为空，返回true
    if (!instr) return true;

    // 根据指令类型判断是否可以进行DCE
    switch (instr->type) {
        case Phi:
        case Binary:
        case Ext:
            return true;
        default:
            return false;
    }
}


bool visitInstructionsDFS(InstructionPtr instr, LoopPtr loop, unordered_map<InstructionPtr, bool>& visitedMap, function<bool(InstructionPtr)> cond, function<void(InstructionPtr)> action) {
    if (!cond(instr))
        return visitedMap[instr];

    action(instr);

    if (!isLoopDeadCodeEliminable(instr) || !loop->contains(instr->basicblock))
        return false;

    assert(instr->reg != nullptr);
    // 递归访问使用指令
    auto use = instr->reg->useHead;
    while (use) {
        assert(use->user != nullptr);
        if (!visitInstructionsDFS(use->user->getSharedThis(), loop, visitedMap, cond, action))
            return false;
        use = use->next;
    }
    return true;
}


void eliminateLoopDeadCode(FunctionPtr func) {
    for (auto loop : func->outerLoops) {
        unordered_set<InstructionPtr> visited;
        unordered_map<InstructionPtr, bool> visitedMap;
        unordered_set<InstructionPtr> instructionsToDelete;
        unordered_set<BasicBlockPtr> changedBasicBlocks;

        for (auto& bb : loop->basicBlocks) {
            for (auto& instr : bb->instructions) {
                if (visited.count(instr))
                    continue;

                vector<InstructionPtr> mayDelete;
                bool canDelete = visitInstructionsDFS(instr, loop, visitedMap,
                    [&](InstructionPtr i) -> bool {
                        return visited.insert(i).second;
                    },
                    [&](InstructionPtr i) -> void {
                        mayDelete.push_back(i);
                    });

                for (auto i : mayDelete) {
                    visitedMap[i] = canDelete;
                }

                // 如果可以删除，更新删除集合和已改变的基本块集合
                if (canDelete) {
                    instructionsToDelete.insert(mayDelete.begin(), mayDelete.end());
                    for (auto i : mayDelete) {
                        changedBasicBlocks.insert(i->basicblock);
                    }
                }
            }
        }

        if (!instructionsToDelete.empty()) {
            for (auto bb : changedBasicBlocks) {
                bb->instructions.erase(
                    std::remove_if(bb->instructions.begin(), bb->instructions.end(),
                                   [&](InstructionPtr instr) { return instructionsToDelete.count(instr) > 0; }),
                    bb->instructions.end());
            }
        }
    }
}