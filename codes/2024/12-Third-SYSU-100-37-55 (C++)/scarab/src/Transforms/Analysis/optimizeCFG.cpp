#include "optimizeCFG.h"

void eliminateUnreachableBlocks(FunctionPtr func) {
    vector<BasicBlockPtr> updatedBasicBlocks;
    unordered_set<BasicBlockPtr> reachableBlocks;

    // 使用栈进行深度优先搜索（DFS）标记所有可达的基本块
    if (!func->basicBlocks.empty()) {
        stack<BasicBlockPtr> toVisit;
        toVisit.push(func->basicBlocks.front());
        while (!toVisit.empty()) {
            BasicBlockPtr currentBlock = toVisit.top();
            toVisit.pop();
            if (reachableBlocks.insert(currentBlock).second) {
                for (const auto& successor : currentBlock->succBasicBlocks) {
                    toVisit.push(successor);
                }
            }
        }
    }

    // 构建新的基本块列表，仅保留可达的基本块
    for (const auto& basicBlock : func->basicBlocks) {
        if (reachableBlocks.find(basicBlock) != reachableBlocks.end()) {
            updatedBasicBlocks.push_back(basicBlock);
        }
        else {
            // 移除不可达基本块的所有后继基本块中的前驱引用
            for (const auto& successor : basicBlock->succBasicBlocks) {
                successor->predBasicBlocks.erase(basicBlock);
            }
        }
    }

    // 更新函数的基本块列表
    func->basicBlocks = updatedBasicBlocks;

    // 清理Phi指令的辅助函数
    auto cleanUpPhiOperands = [](PhiInstruction* phiInstruction, BasicBlockPtr basicBlock) {
        for (auto iter = phiInstruction->from.begin(); iter != phiInstruction->from.end(); ) {
            if (basicBlock->predBasicBlocks.find(iter->second) == basicBlock->predBasicBlocks.end()) {
                Use* use = iter->first->useHead;
                while (use != nullptr) {
                    if (use->user == phiInstruction) {
                        use->rmUse();
                    }
                    use = use->next;
                }
                iter = phiInstruction->from.erase(iter);
            }
            else {
                ++iter;
            }
        }
    };

    // 清理 Phi 指令
    for (auto basicBlock : func->basicBlocks) {
        for (auto it = basicBlock->instructions.begin(); it != basicBlock->instructions.end(); ) {
            if ((*it)->type == Phi) {
                auto phiInstr = dynamic_cast<PhiInstruction*>((*it).get());
                cleanUpPhiOperands(phiInstr, basicBlock);

                // 如果 Phi 指令的前驱仅剩一个，则删除 Phi 指令
                if (phiInstr->from.size() == 1) {
                    replaceVarByVar(phiInstr->reg, phiInstr->from[0].first);
                    deleteUser((*it).get());
                    it = basicBlock->instructions.erase(it);
                }
                else {
                    ++it;
                }
            }
            else {
                break;
            }
        }
    }
}


void mergeBasicBlocks(BasicBlockPtr pred, BasicBlockPtr curr) {
    pred->instructions.pop_back();
    pred->instructions.insert(pred->instructions.end(), curr->instructions.begin(), curr->instructions.end());
    pred->succBasicBlocks = curr->succBasicBlocks;

    // 更新后继基本块的前驱列表
    for (auto succ : curr->succBasicBlocks) {
        succ->predBasicBlocks.erase(curr);
        succ->predBasicBlocks.insert(pred);

        for (auto& instr : succ->instructions) {
            if (instr->type == Phi) {
                auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
                for (auto &phiFrom : phiInstr->from) {
                    if (phiFrom.second == curr) {
                        phiFrom.second = pred;
                    }
                }
            }
            else {
                break;
            }
        }
    }
}

void mergeSinglePredBlocks(FunctionPtr func) {
    for (auto it = func->basicBlocks.begin(); it != func->basicBlocks.end();) {
        auto curr = *it;

        // 判断基本块的前驱数量是否为 1
        if (curr->predBasicBlocks.size() == 1) {
            auto pred = *curr->predBasicBlocks.begin();

            // 检查前驱基本块的最后一条指令是否为无条件跳转指令，且其后继数量为 1
            if (pred->succBasicBlocks.size() == 1) {
                InstructionPtr lastInstr = pred->instructions.back();
                if (auto brInstr = dynamic_cast<BrInstruction*>(lastInstr.get()); brInstr && !brInstr->exp) {
                    mergeBasicBlocks(pred, curr);

                    // 从函数的基本块列表中删除当前基本块，并调整迭代器
                    it = func->basicBlocks.erase(it);
                    continue;
                }
            }
        }

        ++it;
    }
}


void optimizeCFG(FunctionPtr func){
    eliminateUnreachableBlocks(func);
    mergeSinglePredBlocks(func);
}