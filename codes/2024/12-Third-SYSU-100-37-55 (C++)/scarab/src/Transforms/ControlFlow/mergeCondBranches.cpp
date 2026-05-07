#include "mergeCondBranches.h"

bool canMergeFcmp(const FcmpInstruction* currentFcmp, const FcmpInstruction* nextFcmp, float currentRhsValue, float nextRhsValue, bool isTrueBranch) {
    if (isTrueBranch) {
        if (currentFcmp->op == "<") {
            return (nextFcmp->op == ">" && currentRhsValue <= nextRhsValue) ||
                (nextFcmp->op == ">=" && currentRhsValue < nextRhsValue);
        }
        else if (currentFcmp->op == "<=") {
            return (nextFcmp->op == ">" && currentRhsValue < nextRhsValue) ||
                (nextFcmp->op == ">=" && currentRhsValue <= nextRhsValue);
        }
        else if (currentFcmp->op == ">") {
            return (nextFcmp->op == "<" && currentRhsValue >= nextRhsValue) ||
                (nextFcmp->op == "<=" && currentRhsValue > nextRhsValue);
        }
        else if (currentFcmp->op == ">=") {
            return (nextFcmp->op == "<" && currentRhsValue > nextRhsValue) ||
                (nextFcmp->op == "<=" && currentRhsValue >= nextRhsValue);
        }
    }
    else {
        if (currentFcmp->op == "<") {
            return (nextFcmp->op == ">" && currentRhsValue > nextRhsValue) ||
                (nextFcmp->op == ">=" && currentRhsValue >= nextRhsValue);
        }
        else if (currentFcmp->op == "<=") {
            return (nextFcmp->op == ">" && currentRhsValue >= nextRhsValue) ||
                (nextFcmp->op == ">=" && currentRhsValue >= nextRhsValue);
        }
        else if (currentFcmp->op == ">") {
            return (nextFcmp->op == "<" && currentRhsValue < nextRhsValue) ||
                (nextFcmp->op == "<=" && currentRhsValue <= nextRhsValue);
        }
        else if (currentFcmp->op == ">=") {
            return (nextFcmp->op == "<" && currentRhsValue <= nextRhsValue) ||
                (nextFcmp->op == "<=" && currentRhsValue <= nextRhsValue);
        }
    }
    if (currentFcmp->op == "==") {
        return nextFcmp->op == "!=" && currentRhsValue == nextRhsValue;
    }
    else if (currentFcmp->op == "!=") {
        return nextFcmp->op == "==" && currentRhsValue == nextRhsValue;
    }
    return false;
}

bool mergeCondBranches(FunctionPtr func) {
    unordered_set<BasicBlockPtr> processedBlocks;
    bool changed = false;
    for (auto& basicBlock: func->basicBlocks) {
        if (!processedBlocks.insert(basicBlock).second) continue;

        for (size_t i = 0; i < basicBlock->instructions.size(); i++) {
            auto instr = basicBlock->instructions[i];
            if (instr->type == Fcmp) {
                auto currentFcmp = dynamic_cast<FcmpInstruction *>(instr.get());
                auto constRhs = dynamic_cast<Const *>(currentFcmp->b.get());
                if (!constRhs) continue;
                float currentRhsValue = constRhs->floatVal;
                
                if (i + 1 >= basicBlock->instructions.size() || basicBlock->instructions[i + 1]->type != Br) continue;

                auto brInstr = dynamic_cast<BrInstruction *>(basicBlock->instructions[i + 1].get());
                if (brInstr->exp != currentFcmp->reg) continue;

                auto trueBlock = func->LabelToBBMap[brInstr->label1];
                auto falseBlock = func->LabelToBBMap[brInstr->label2];
                if (!trueBlock || !falseBlock) continue;
                
                auto mergeIfPossible = [&](BasicBlockPtr block, bool isTrueBranch) {
                    if (block->instructions.size() < 2) return false;
                    auto nextInstr = block->instructions.end()[-2];
                    auto nextFcmp = dynamic_cast<FcmpInstruction*>(nextInstr.get());
                    if (!nextFcmp || block->instructions.back()->type != Br) return false;
                    auto nextRhs = dynamic_cast<Const*>(nextFcmp->b.get());
                    if (!nextRhs) return false;

                    float nextRhsValue = nextRhs->floatVal;
                    if (canMergeFcmp(currentFcmp, nextFcmp, currentRhsValue, nextRhsValue, isTrueBranch)) {
                        brInstr->exp = Const::createConstant(Type::getBool(), isTrueBranch ? 0 : 1, "if_combined_1");
                        auto nextBrInstr = dynamic_cast<BrInstruction*>(block->instructions.back().get());
                        nextBrInstr->exp = Const::createConstant(Type::getBool(), isTrueBranch ? 0 : 1, "if_combined_1");
                        processedBlocks.insert(block);
                        changed = true;
                        return true;
                    }
                    return false;
                };

                if (mergeIfPossible(trueBlock, true) || mergeIfPossible(falseBlock, false)) {
                    break;
                }
            }
        }
    }
    return changed;
}