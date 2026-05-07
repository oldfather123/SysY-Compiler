#include "branch2select.h"

void branch2select(FunctionPtr func) {
    for (auto it = func->basicBlocks.begin(); it != func->basicBlocks.end(); ++it) {
        auto bb = *it;
        if (auto condBrInstr = dynamic_cast<BrInstruction*>(bb->instructions.back().get())) {
            if (condBrInstr->exp && bb->succBasicBlocks.size() == 2) {
                auto succ1 = func->LabelToBBMap[condBrInstr->label1];
                auto succ2 = func->LabelToBBMap[condBrInstr->label2];
                if (succ1->predBasicBlocks.size() == 1 && succ2->predBasicBlocks.size() == 1 && 
                    succ1->succBasicBlocks.size() == 1 && succ2->succBasicBlocks.size() == 1) {
                    auto it1 = succ1->succBasicBlocks.begin();
                    auto it2 = succ2->succBasicBlocks.begin();
                    auto merge1 = *it1;
                    auto merge2 = *it2;
                    if (merge1 == merge2) {
                        auto mergeBB = merge1;
                        if (auto phiInstr = dynamic_cast<PhiInstruction*>(mergeBB->instructions.front().get())) {
                            if (phiInstr->from.size() == 2) {
                                int flag = 0;
                                for (auto& from : phiInstr->from) {
                                    if (from.second == succ1 || from.second == succ2) {
                                        flag++;
                                    }
                                }
                                if (flag == 2) {
                                    // 创建select指令并替换phi指令
                                    auto val1 = phiInstr->from[0].first;
                                    auto val2 = phiInstr->from[1].first;
                                    if (val1->type->isInt() && val2->type->isInt()) {
                                        bool canMerge = true;
                                        for (auto inst : succ1->instructions) {
                                            if (inst->type == Store || inst->type == Call) {
                                                canMerge = false;
                                                break;
                                            }
                                        }
                                        for (auto inst : succ2->instructions) {
                                            if (inst->type == Store || inst->type == Call) {
                                                canMerge = false;
                                                break;
                                            }
                                        }
                                        if (canMerge) {
                                            auto selectInstr = std::make_shared<SelectInstruction>(bb, condBrInstr->exp, val1, val2);
                                            mergeBB->instructions.front() = selectInstr;

                                            auto phiVal = phiInstr->reg;
                                            auto useHead = phiVal->useHead;
                                            while (useHead) {
                                                auto use = useHead->user;
                                                use->replaceOperand(phiVal, selectInstr->reg);
                                                useHead = useHead->next;
                                            }

                                            // 移除br指令
                                            bb->instructions.pop_back();
                                            succ1->instructions.pop_back();
                                            succ2->instructions.pop_back();

                                            if (!succ1->instructions.empty()) {
                                                bb->instructions.insert(bb->instructions.end(), succ1->instructions.begin(), succ1->instructions.end());
                                            }
                                            if (!succ2->instructions.empty()) {
                                                bb->instructions.insert(bb->instructions.end(), succ2->instructions.begin(), succ2->instructions.end());
                                            }
                                            for (auto instr : mergeBB->instructions) {
                                                if (auto phi = dynamic_cast<PhiInstruction*>(instr.get())) {
                                                    if (phi->from.size() == 2) {
                                                        int count = 0;
                                                        for (auto& from : phi->from) {
                                                            if (from.second == succ1 || from.second == succ2) {
                                                                count++;
                                                            }
                                                        }
                                                        if (count == 2) {
                                                            // 创建select指令并替换phi指令
                                                            auto val1 = phi->from[0].first;
                                                            auto val2 = phi->from[1].first;
                                                            auto selectInstr = std::make_shared<SelectInstruction>(mergeBB, condBrInstr->exp, val1, val2);
                                                            mergeBB->insertInstructionBefore(selectInstr, instr);
                                                            mergeBB->removeInstruction(instr);

                                                            auto phiVal = phi->reg;
                                                            auto useHead = phiVal->useHead;
                                                            while (useHead) {
                                                                auto use = useHead->user;
                                                                use->replaceOperand(phiVal, selectInstr->reg);
                                                                useHead = useHead->next;
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            for (auto& succ : mergeBB->succBasicBlocks) {
                                                succ->predBasicBlocks.erase(mergeBB);
                                                succ->predBasicBlocks.insert(bb);
                                                bb->succBasicBlocks.insert(succ);
                                                for(auto instr : succ->instructions){
                                                    if(instr->type == Phi){
                                                        auto phiInstr = dynamic_cast<PhiInstruction*> (instr.get());
                                                        for (auto &phiFrom : phiInstr->from) {
                                                            if (phiFrom.second == mergeBB) {
                                                                phiFrom.second = bb;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            // 将 mergeBB 中的其他指令合并到 bb 中
                                            bb->instructions.insert(bb->instructions.end(), mergeBB->instructions.begin(), mergeBB->instructions.end());

                                            bb->succBasicBlocks.erase(succ1);
                                            bb->succBasicBlocks.erase(succ2);

                                            // 移除 then_block、else_block 和 mergeBB
                                            func->basicBlocks.erase(std::remove(func->basicBlocks.begin(), func->basicBlocks.end(), succ1), func->basicBlocks.end());
                                            func->basicBlocks.erase(std::remove(func->basicBlocks.begin(), func->basicBlocks.end(), succ2), func->basicBlocks.end());
                                            func->basicBlocks.erase(std::remove(func->basicBlocks.begin(), func->basicBlocks.end(), mergeBB), func->basicBlocks.end());

                                            // 调整迭代器
                                            it = func->basicBlocks.begin();                                            
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}