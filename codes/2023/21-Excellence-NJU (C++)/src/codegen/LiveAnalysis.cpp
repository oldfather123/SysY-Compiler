#include "codegen/LiveAnalysis.hpp"
#include "codegen/MachineBlock.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"
#include "codegen/MachineUnit.hpp"
#include "BasicBlock.h"
#include "Function.h"

#include <algorithm>

void LiveAnalysis::pass(MachineUnit *unit) {

    for (auto func : unit->func_list) {
        calcLiveDefUse(func);
        calcLiveInOut(func);
    }
}

void LiveAnalysis::calcLiveDefUse(MachineFunction *mFunc) {
    for (auto mBlock : mFunc->block_list) {
        for (auto inst : mBlock->inst_list) {
            auto &live_use = mBlock->live_use;
            auto &live_def = mBlock->live_def;

            auto &f_live_use = mBlock->f_live_use;
            auto &f_live_def = mBlock->f_live_def;

            for (auto use : inst->use_list) {
                switch (use->type) {
                    case MachineOperand::VREG: 
                        if (live_def.find(use->vreg_name) == live_def.end())
                            live_use.insert(use->vreg_name);
                        break;
                    case MachineOperand::FVREG:
                        if (f_live_def.find(use->vreg_name) == f_live_def.end())
                            f_live_use.insert(use->vreg_name);
                        break;
                    default:
                        continue;
                }
            }
            
            for (auto def : inst->def_list) {
                // if (def->type != MachineOperand::VREG) continue;
                // const auto &vreg = def->vreg_name;
                // live_def.insert(vreg);
                switch (def->type) {
                    case MachineOperand::VREG:
                        live_def.insert(def->vreg_name);
                        break;
                    case MachineOperand::FVREG:
                        f_live_def.insert(def->vreg_name);
                        break;
                    default:
                        continue;
                }
            }
        }
    }
}

void LiveAnalysis::calcLiveInOut(MachineFunction *mFunc) {
    bool change;
    auto reverse_post_order = std::move(mFunc->IR_func->getReversePostOrder());
    std::vector<BasicBlock *> order(reverse_post_order.rbegin(), reverse_post_order.rend());

    do {
        change = false;
        // 逆序遍历mFunc->block_list(当前函数的所有block)
        for (auto block : order) {
            MachineBlock *mBlock = block->mBlock;
            auto old_live_out(mBlock->live_out);
            mBlock->live_out.clear();
            for (auto bb : mBlock->bb->succ) {
                auto succ = bb->mBlock;
                mBlock->live_out.insert(succ->live_in.begin(), succ->live_in.end());
            }
            if (old_live_out != mBlock->live_out) change = true;

            auto tmp(mBlock->live_use);
            // 求差集和并集
            std::set_difference(mBlock->live_out.begin(), mBlock->live_out.end(),
                                mBlock->live_def.begin(), mBlock->live_def.end(),
                                std::inserter(tmp, tmp.begin()));
            mBlock->live_in = std::move(tmp);
        }
    } while (change);
    
    do {
        change = false;
        // 逆序遍历mFunc->block_list(当前函数的所有block)
        for (auto block : order) {
            MachineBlock *mBlock = block->mBlock;
            auto old_f_live_out(mBlock->f_live_out);
            mBlock->f_live_out.clear();
            for (auto bb : mBlock->bb->succ) {
                auto succ = bb->mBlock;
                mBlock->f_live_out.insert(succ->f_live_in.begin(), succ->f_live_in.end());
            }
            if (old_f_live_out != mBlock->f_live_out) change = true;

            auto tmp(mBlock->f_live_use);
            // 求差集和并集
            std::set_difference(mBlock->f_live_out.begin(), mBlock->f_live_out.end(),
                                mBlock->f_live_def.begin(), mBlock->f_live_def.end(),
                                std::inserter(tmp, tmp.begin()));
            mBlock->f_live_in = std::move(tmp);
        }
    } while (change);
}