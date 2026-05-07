#include "../../include/ir/merge_bb.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "../../include/ir/cfg.h"
#include "../../include/ir/llvm_ir.h"

namespace llvm_ir {
namespace merge_bb {

// Module-level entry point for basic block merge pass
bool runOnModule(Module& M) {
    bool hasChanged = false;

    std::cout << "[CFG-MergeBB] Starting basic block merge optimization on module" << std::endl;

    for (auto& func : M.functions) {
        if (!func->isDeclaration()) {
            // 记录合并前的块数量
            size_t beforeCount = func->basicBlocks.size();

            MergeBasicBlocks(*func);

            // 检查是否有变化
            size_t afterCount = func->basicBlocks.size();
            if (afterCount < beforeCount) {
                hasChanged = true;
            }
        }
    }

    if (hasChanged) {
        std::cout << "[CFG-MergeBB] Basic block merge optimization completed with changes" << std::endl;
    } else {
        std::cout << "[CFG-MergeBB] Basic block merge optimization completed - no changes needed" << std::endl;
    }

    return hasChanged;
}

void MergeBasicBlocks(Function& F, std::vector<BasicBlock*> &included, std::vector<BasicBlock*> &excluded) {
    std::cout << "[CFG-MergeBB] >>> Starting basic block merge optimization" << std::endl;

    // 初始建立CFG关系
    cfg::buildCFG(F);

    bool changed = true;
    int mergeCount = 0;

    while (changed) {
        changed = false;
        // 每轮迭代开始时重建一次CFG
        // cfg::buildCFG(F);

        // for (auto it = F.basicBlocks.begin(); it != F.basicBlocks.end(); ++it) {
        //     std::cout << "[CFG-MergeBB] Checking block: " << (*it)->label << std::endl;
        // }

        // 如果不这么做：
        // [DEBG] [DEBUG] >>>> bbB: transfer_block_1 Phi: %s1_i_phi_0_1_1 = phi i32 [ %t_3_1, %doubleWhile_label_19_1 ], 1 incoming values,
        // [DEBG] [DEBUG] >>>> bbB: transfer_block_1 Phi: %s1_j_phi_0_1_1 = phi i32 [ %t_10_1, %doubleWhile_label_19_1 ], 1 incoming values,
        // [DEBG] [DEBUG] !!!bbB: transfer_block_1 Phi: %s1_i_phi_0_1_1 = phi i32 [ %t_3_1, %doubleWhile_label_19_1 ], incoming value: %t_3_1 = add i32 %s1_i_phi_0_1, 30, from block: doubleWhile_label_19_1
        // [DEBG] [DEBUG] bbB: transfer_block_1 Phi: %s1_i_phi_0_1_1 = phi i32 [ %t_3_1, %doubleWhile_label_19_1 ], 1 incoming values,

        // 遍历所有基本块寻找可合并的块对
        for (auto it = F.basicBlocks.begin(); it != F.basicBlocks.end(); ++it) {
            BasicBlock* bbA = it->get();

            // 检查合并条件
            // 1. bbA只有一个后继
            // for (auto& x : bbA->successors) {
            //     std::cout << "[DEBUG] bbA: " << bbA->label << " successor: " << x->label << ", ";
            // }
            // std::cout << std::endl;
            if (bbA->successors.size() != 1) {
                continue;
            }

            BasicBlock* bbB = nullptr;
            if (it + 1 == F.basicBlocks.end()) {
                // 如果没有下一个块，跳过
                break;
            } else {
                bbB = (it + 1)->get();  // 获取 basicBlocks的下一个基本块
            }

            if (bbA->successors[0] != bbB) {
                // 后继不是下一个块，跳过
                continue;
            }

            // 2. bbB只有一个前驱
            // for (auto& x : bbB->predecessors) {
            //     std::cout << "[DEBUG] bbB: " << bbB->label << " predecessor: " << x->label << ", ";
            // }
            // 检查B的Phi指令，如果有只有一个来源的Phi那么replaceAllUsesWith
            // for (auto& I: bbB->phi_instructions) {
            //     std::cout << "[DEBUG] >>>> bbB: " << bbB->label << " Phi: " << I->toString() << ", " << I->incoming_values.size() << " incoming values, " << std::endl;
            // }

            // 使用每个候选对独立的待删集合，避免重复删除
            std::vector<Instruction*> toDelete;
            for (auto& I : bbB->phi_instructions) {
                // for (auto & inc : I->incoming_values) {
                //     std::cout << "[DEBUG] !!!bbB: " << bbB->label << " Phi: " << I->toString() << ", incoming value: " << inc.first->toString() << ", from block: " << inc.second->label << std::endl;
                // }
                // std::cout << "[DEBUG] bbB: " << bbB->label << " Phi: " << I->toString() << ", " << I->incoming_values.size() << " incoming values, " << std::endl;
                if (I->incoming_values.size() == 1) {
                    // 只有一个来源的Phi指令，直接替换
                    I->replaceAllUsesWith(I->incoming_values.begin()->first);
                    // I->removeFromParent();
                    toDelete.push_back(I.get());  // 标记为删除
                }
                // std::cout << "reach here vvvv" << std::endl;
            }

            for (auto* inst : toDelete) {
                if (!inst || !inst->parent) continue; // 已被删除或无父指针，跳过
                inst->removeFromParent();  // 删除标记的指令
            }
            toDelete.clear();
            
            // std::cout << "reach here" << std::endl;
            if (bbB->predecessors.size() != 1) {
                continue;
            }

            // 3. bbA的终止指令必须是无条件跳转
            Instruction* terminator = bbA->getTerminator();
            if (!terminator)
                continue;

            auto br = dynamic_cast<BranchInst*>(terminator);
            if (!br || br->isConditional())
                continue;  // 必须是无条件跳转

            // 4. 跳转目标必须是bbB
            if (br->trueLabel != bbB->label)
                continue;

            // 5. 不能合并入口块到另一个块（保持入口块的特殊地位）
            if (bbB == F.getEntryBlock())
                continue;

            // 6. 检查included约束：如果included非空，bbA和bbB都必须在included中
            if (!included.empty()) {
                // bool bbA_included = std::find(included.begin(), included.end(), bbA) != included.end();
                bool bbB_included = std::find(included.begin(), included.end(), bbB) != included.end();
                // if (!bbA_included || !bbB_included) {
                if (!bbB_included) {
                    // std::cout << "[CFG-MergeBB] Skipping merge: blocks not in included list" << std::endl;
                    continue;
                }
            }

            // 7. 检查excluded约束：bbA和bbB都不能在excluded中
            if (!excluded.empty()) {
                // bool bbA_excluded = std::find(excluded.begin(), excluded.end(), bbA) != excluded.end();
                bool bbB_excluded = std::find(excluded.begin(), excluded.end(), bbB) != excluded.end();
                // if (bbA_excluded || bbB_excluded) {
                if (bbB_excluded) {
                    // std::cout << "[CFG-MergeBB] Skipping merge: blocks in excluded list" << std::endl;
                    continue;
                }
            }

            // std::cout << "[CFG-MergeBB] Merging block %" << bbA->label << " into %" << bbB->label << std::endl;

            // 执行合并
            mergeBlocks(bbA, bbB, F);
            mergeCount++;
            changed = true;
            // 每次合并后重建CFG，避免下一次迭代使用过期关系
            // cfg::buildCFG(F);
            break;  // 重新开始遍历，因为迭代器可能失效
        }
    }

    if (mergeCount > 0) {
        cfg::buildCFG(F);
        cfg::rebuildUseDefChainsOnFunction(F, /*debug=*/false);
        std::cout << "[CFG-MergeBB] Merged " << mergeCount << " basic block pairs" << std::endl;
    } else {
        std::cout << "[CFG-MergeBB] No basic blocks to merge" << std::endl;
    }

    // 合并完成后，重建CFG和use-def链
    // cfg::buildCFG(F);
}

void mergeBlocks(BasicBlock* bbA, BasicBlock* bbB, Function& F) {
    // std::cout << "[CFG-MergeBB] Merging blocks: " << bbA->label << " <- " << bbB->label << std::endl;
    // 前置条件检查
    assert(bbA != nullptr && "bbA cannot be null");
    assert(bbB != nullptr && "bbB cannot be null");
    assert(bbA->successors.size() == 1 && "bbA must have exactly one successor");
    assert(bbB->predecessors.size() == 1 && "bbB must have exactly one predecessor");
    assert(bbA->successors[0] == bbB && "bbA's successor must be bbB");
    assert(bbB->predecessors[0] == bbA && "bbB's predecessor must be bbA");

    // 检查bbA的终止指令
    Instruction* terminator = bbA->getTerminator();
    assert(terminator != nullptr && "bbA must have a terminator instruction");
    auto br = dynamic_cast<BranchInst*>(terminator);
    assert(br != nullptr && "bbA's terminator must be a branch instruction");
    assert(!br->isConditional() && "bbA's branch must be unconditional");
    assert(br->trueLabel == bbB->label && "bbA's branch target must be bbB");

    // 1. 移除bbA的终止指令（无条件跳转）并手动移除use关系
    if (!bbA->instructions.empty()) {
        auto& lastInst = bbA->instructions.back();
        if (auto br = dynamic_cast<BranchInst*>(lastInst.get())) {
            // 手动移除use关系
            for (Value* operand : br->operands) {
                if (operand) {
                    operand->removeUse(br);
                }
            }
            bbA->instructions.pop_back();
        }
    }

    // 2. bbB不应该有PHI指令，因为它只有一个前驱(bbA)
    assert(bbB->phi_instructions.empty() && "bbB should not have PHI instructions when it has only one predecessor");

    // 3. 将bbB的所有普通指令移动到bbA并更新parent指针
    for (auto& inst : bbB->instructions) {
        inst->parent = bbA;
    }

    bbA->instructions.insert(bbA->instructions.end(), std::make_move_iterator(bbB->instructions.begin()), std::make_move_iterator(bbB->instructions.end()));
    bbB->instructions.clear();
    cfg::rebuildUseDefChainsOnFunction(F, /*debug=*/false); //TOOD 这里应该不用完全重建

    // 4. 手动维护CFG关系：更新bbA的后继关系
    bbA->successors = bbB->successors;

    // 5. 手动维护CFG关系：更新bbB的后继块的前驱关系
    for (BasicBlock* successor : bbB->successors) {
        // 将successor的前驱从bbB改为bbA
        for (auto& pred : successor->predecessors) {
            if (pred == bbB) {
                pred = bbA;
                break;
            }
        }

        // 更新successor中PHI指令的前驱块引用
        for (auto& phi : successor->phi_instructions) {
            for (auto& incoming : phi->incoming_values) {
                if (incoming.second == bbB) {
                    incoming.second = bbA;
                }
            }
        }
    }
    // std::cout << "[Merge] Last Inst bbA: " << (bbA->instructions.empty() ? "None" : bbA->instructions.back()->toString()) << std::endl;

    // 6. 手动维护跳转指令：更新函数中所有跳转指令的目标标签
    // for (auto& bb : F.basicBlocks) {
    //     for (auto& inst : bb->instructions) {
    //         if (auto br = dynamic_cast<BranchInst*>(inst.get())) {
    //             if (br->trueLabel == bbB->label) {
    //                 br->trueLabel = bbA->label;
    //             }
    //             if (!br->falseLabel.empty() && br->falseLabel == bbB->label) {
    //                 br->falseLabel = bbA->label;
    //             }
    //         }
    //     }
    // }

    // 7. 手动清理CFG关系：清理bbB的前驱和后继关系
    bbB->predecessors.clear();
    bbB->successors.clear();

    // 8. 从函数中删除bbB
    auto removed_count = std::count_if(F.basicBlocks.begin(), F.basicBlocks.end(), [bbB](const std::unique_ptr<BasicBlock>& bb) { return bb.get() == bbB; });
    assert(removed_count == 1 && "bbB should exist exactly once in the function");

    F.basicBlocks.erase(std::remove_if(F.basicBlocks.begin(), F.basicBlocks.end(), [bbB](const std::unique_ptr<BasicBlock>& bb) { return bb.get() == bbB; }), F.basicBlocks.end());

    // 验证合并后的状态
    assert(std::find_if(F.basicBlocks.begin(), F.basicBlocks.end(), [bbB](const std::unique_ptr<BasicBlock>& bb) { return bb.get() == bbB; }) == F.basicBlocks.end() && "bbB should be removed from function");
}

// 向后兼容的重载版本
void MergeBasicBlocks(Function& F) {
    std::vector<BasicBlock*> included;  // 空向量，表示没有included限制
    std::vector<BasicBlock*> excluded;  // 空向量，表示没有excluded限制
    MergeBasicBlocks(F, included, excluded);
}

}  // namespace merge_bb
}  // namespace llvm_ir
