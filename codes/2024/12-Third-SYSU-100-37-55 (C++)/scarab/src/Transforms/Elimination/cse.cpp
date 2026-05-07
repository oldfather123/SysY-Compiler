#include "cse.h"

bool isNotCandidateForCSE(InsID type) {
    return (type == Br || type == Return || type == Alloca
        || type == Phi || type == Icmp || type == Fcmp);
}


bool canMergeConstants(Const* const1, Const* const2) {
    if (!const1 || !const2) return false;

    if (const1->type->isInt() && const2->type->isInt()) {
        return const1->intVal == const2->intVal;
    }

    if (const1->type->isFloat() && const2->type->isFloat()) {
        return const1->floatVal == const2->floatVal;
    }

    if (const1->type->isBool() && const2->type->isBool()) {
        return const1->boolVal == const2->boolVal;
    }

    return false;
}


bool possiblySameGEP(Instruction *instr1, Instruction *instr2) {
    auto gepInstr1 = dynamic_cast<GetElementPtrInstruction *>(instr1);
    auto gepInstr2 = dynamic_cast<GetElementPtrInstruction *>(instr2);

    // 检查是否都是GEP指令，并且allIndex大小相同，base相同
    if (!gepInstr1 || !gepInstr2 || gepInstr1->allIndex.size() != gepInstr2->allIndex.size() || gepInstr1->base != gepInstr2->base) {
        return false;
    }

    for (size_t idx = 0; idx < gepInstr1->allIndex.size(); idx++) {
        if (gepInstr1->allIndex[idx] == gepInstr2->allIndex[idx]) {
            continue;
        }

        // 检查是否为常量，并比较常量值
        auto constIdx1 = dynamic_cast<Const *>(gepInstr1->allIndex[idx].get());
        auto constIdx2 = dynamic_cast<Const *>(gepInstr2->allIndex[idx].get());

        if (!constIdx1 || !constIdx2) {
            return true;
        }
        else if (constIdx1->intVal != constIdx2->intVal) {
            return false;
        }
    }

    return true;
}


void replaceInstruction(ValuePtr oldvalue, ValuePtr newvalue, InstructionPtr instr, BasicBlockPtr succBlock, unordered_map<InstructionPtr, BasicBlockPtr>& deletelist, unordered_set<BasicBlockPtr>& modifiedBlocks) {
    replaceVarByVar(oldvalue, newvalue);
    deletelist[instr] = succBlock;
    modifiedBlocks.insert(succBlock);
    deleteUser(newvalue);
}


void cse(FunctionPtr func){
    for (auto basicblock : func->basicBlocks) {
        unordered_map<InstructionPtr, BasicBlockPtr> deletelist;
        unordered_set<BasicBlockPtr> modifiedBlocks;
        unordered_map<BrInstruction*, bool> branchContinue;
        unordered_map<BasicBlockPtr, bool> blockContinue;

        // 收集当前基本块及其后继块
        vector<BasicBlockPtr> successorBlocks{basicblock};
        successorBlocks.insert(successorBlocks.end(), basicblock->succBasicBlocks.begin(), basicblock->succBasicBlocks.end());

        for (size_t i = 0; i < basicblock->instructions.size(); i++) {
            auto currInstr = basicblock->instructions[i];
            if (deletelist.count(currInstr) || isNotCandidateForCSE(currInstr->type)) continue;
            bool shouldBreak = false;

            for (auto succBB: successorBlocks) {
                size_t j = (succBB == basicblock ? i + 1 : 0);
                if (blockContinue.count(succBB) && !blockContinue[succBB]) continue;

                for (; j < succBB->instructions.size(); j++) {
                    auto nextInstr = succBB->instructions[j];
                    if ((currInstr->type == Load || currInstr->type == Store) && nextInstr->type == Call) {
                        if (succBB == basicblock) {
                            shouldBreak = true;
                        }
                        break;
                    }
                    if (nextInstr->type == Br) {
                        auto brInstr = static_cast<BrInstruction*>(nextInstr.get());
                        if (branchContinue.find(brInstr) == branchContinue.end()) {
                            auto nextBB = func->LabelToBBMap[brInstr->label1];
                            blockContinue[nextBB] = (nextBB->predBasicBlocks.size() == 1);
                            if (brInstr->label2) {
                                nextBB = func->LabelToBBMap[brInstr->label2];
                                blockContinue[nextBB] = blockContinue[nextBB] || (nextBB->predBasicBlocks.size() == 1);
                            }
                            branchContinue[brInstr] = blockContinue[nextBB];
                        }
                        if (!branchContinue[brInstr]) {
                            if (succBB == basicblock) {
                                shouldBreak = true;
                            }
                        }
                    }
                    if (shouldBreak) break;
                    if (deletelist.count(nextInstr)) continue;

                    if (currInstr->type == nextInstr->type) {
                        switch (currInstr->type) {
                            case Binary: {
                                auto binaryInstr1 = static_cast<BinaryInstruction*>(currInstr.get());
                                auto binaryInstr2 = static_cast<BinaryInstruction*>(nextInstr.get());
                                if (binaryInstr1->op == binaryInstr2->op) {
                                    if (binaryInstr1->a == binaryInstr2->a && binaryInstr1->b == binaryInstr2->b) {
                                        replaceInstruction(binaryInstr2->reg, binaryInstr1->reg, nextInstr, succBB, deletelist, modifiedBlocks);
                                    }
                                    else {
                                        auto constA1 = dynamic_cast<Const*>(binaryInstr1->a.get());
                                        auto constA2 = dynamic_cast<Const*>(binaryInstr2->a.get());
                                        auto constB1 = dynamic_cast<Const*>(binaryInstr1->b.get());
                                        auto constB2 = dynamic_cast<Const*>(binaryInstr2->b.get());
                                        if ((canMergeConstants(constA1, constA2) && binaryInstr1->b == binaryInstr2->b) ||
                                            (canMergeConstants(constB1, constB2) && binaryInstr1->a == binaryInstr2->a)) {
                                            replaceInstruction(binaryInstr2->reg, binaryInstr1->reg, nextInstr, succBB, deletelist, modifiedBlocks);
                                        }
                                    }
                                }
                                break;
                            }
                            case Store: {
                                auto storeInstr1 = static_cast<StoreInstruction*>(currInstr.get());
                                auto storeInstr2 = static_cast<StoreInstruction*>(nextInstr.get());
                                if (succBB != basicblock && basicblock->succBasicBlocks.size() > 1) {
                                    shouldBreak = true;
                                    break;
                                }
                                if (storeInstr1->des == storeInstr2->des || possiblySameGEP(storeInstr1->des->I, storeInstr2->des->I)) {
                                    shouldBreak = true;
                                    break;
                                }
                                if (storeInstr1->des == storeInstr2->des) {
                                    deletelist[currInstr] = basicblock;
                                    modifiedBlocks.insert(basicblock);
                                    rmInsUse(currInstr.get(), storeInstr1->des);
                                    rmInsUse(currInstr.get(), storeInstr1->value);
                                }
                                break;
                            }
                            case Load: {
                                auto loadInstr1 = static_cast<LoadInstruction*>(currInstr.get());
                                auto loadInstr2 = static_cast<LoadInstruction*>(nextInstr.get());
                                if (loadInstr1->from == loadInstr2->from) {
                                    replaceInstruction(loadInstr2->to, loadInstr1->to, nextInstr, succBB, deletelist, modifiedBlocks);
                                }
                                break;
                            }
                            case GEP: {
                                auto gepInstr1 = static_cast<GetElementPtrInstruction*>(currInstr.get());
                                auto gepInstr2 = static_cast<GetElementPtrInstruction*>(nextInstr.get());
                                if (gepInstr1->from == gepInstr2->from && gepInstr1->getOperandCount() == gepInstr2->getOperandCount()) {
                                    bool matched = true;
                                    for (size_t k = 0; k < gepInstr1->index.size(); k++) {
                                        if (gepInstr1->index[k] != gepInstr2->index[k]) {
                                            auto const0 = dynamic_cast<Const *>(gepInstr1->index[k].get());
                                            auto const1 = dynamic_cast<Const *>(gepInstr2->index[k].get());
                                            if (const0 == nullptr || const1 == nullptr || const0->intVal != const1->intVal) {
                                                matched = false;
                                                break;
                                            }
                                        }
                                    }
                                    if (matched) {
                                        replaceInstruction(gepInstr2->reg, gepInstr1->reg, nextInstr, succBB, deletelist, modifiedBlocks);
                                    }
                                }
                                break;
                            }
                            case Ext: {
                                auto extInstr1 = static_cast<ExtInstruction*>(currInstr.get());
                                auto extInstr2 = static_cast<ExtInstruction*>(nextInstr.get());
                                if (extInstr1->from == extInstr2->from && extInstr1->to->ID == extInstr2->to->ID) {
                                    replaceInstruction(extInstr2->reg, extInstr1->reg, nextInstr, succBB, deletelist, modifiedBlocks);
                                }
                                break;
                            }
                        }
                    }
                    else if (currInstr->type == Store && nextInstr->type == Load) {
                        auto storeInstr = static_cast<StoreInstruction*>(currInstr.get());
                        auto loadInstr = static_cast<LoadInstruction*>(nextInstr.get());
                        if (storeInstr->des == loadInstr->from) {
                            replaceInstruction(loadInstr->to, storeInstr->value, nextInstr, succBB, deletelist, modifiedBlocks);
                        }
                    }
                    else if (currInstr->type == Load && nextInstr->type == Store) {
                        auto loadInstr = static_cast<LoadInstruction*>(currInstr.get());
                        auto storeInstr = static_cast<StoreInstruction*>(nextInstr.get());
                        if (loadInstr->from == storeInstr->des || possiblySameGEP(loadInstr->from->I, storeInstr->des->I)) {
                            shouldBreak = true;
                            break;
                        }
                    }
                }
            }
        }
        for (auto modifiedBlock: modifiedBlocks) {
            vector<InstructionPtr> newInstructions;
            for (auto& instr : modifiedBlock->instructions) {
                if (deletelist.count(instr) == 0) {
                    newInstructions.push_back(instr);
                }
            }
            modifiedBlock->instructions = newInstructions;
        }
    }
}