#include "basic_block_optimization.h"

void basic_block_optimization(FunctionPtr func) {
        for(auto it = func->basicBlocks.begin();it!=func->basicBlocks.end();){
        auto bb = *it;
        //前缀为1
        // 新增检查：前驱块的最后一条指令是带条件的跳转指令，后继块的指令只有一条无条件跳转指令
        if (bb->predBasicBlocks.size() == 1) {
            auto pred = *bb->predBasicBlocks.begin();
            if (auto condBrInstr = dynamic_cast<BrInstruction*>(pred->instructions.back().get())) {
                if (condBrInstr->exp && bb->instructions.size() == 1 && !(bb->isPreHeader)) {
                    if (auto brInstr = dynamic_cast<BrInstruction*>(bb->instructions.front().get())) {
                        if (!brInstr->exp) {
                            // 更新前驱块的跳转指令目标
                            if (condBrInstr->label1 == bb->label) {
                                condBrInstr->label1 = brInstr->label1;
                            }
                            if (condBrInstr->label2 == bb->label) {
                                condBrInstr->label2 = brInstr->label1;
                            }

                            // 更新后继块的前驱块
                            auto succ = func->LabelToBBMap[brInstr->label1];
                            succ->predBasicBlocks.erase(bb);
                            succ->predBasicBlocks.insert(pred);

                            // 更新Phi指令的来源块
                            for (auto& instr : succ->instructions) {
                                if (instr->type == Phi) {
                                    auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
                                    for (auto& from : phiInstr->from) {
                                        if (from.second == bb) {
                                            from.second = pred;
                                        }
                                    }
                                }
                                else {
                                    break;
                                }
                            }
                            it = func->basicBlocks.erase(it);
                            continue;
                        }
                    }
                }
            }
        }
        it++;
    }
}