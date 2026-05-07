#include "backend/opt/cfg_opt/simplify_cfg.hpp"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace backend::opt::cfg_opt {

void SimplifyCFG::run(backend::riscv::RVModule *module) {
    for (const auto &[name, function] : module->get_functions()) {
        if (function->is_external()) continue;
        simplify_cfg(function);
    }
}

bool SimplifyCFG::redirect_goto(backend::riscv::RVFunction *func) {
    // logger::INFO("redirect_goto");
    std::unordered_map<backend::riscv::RVBasicBlock *, backend::riscv::RVBasicBlock *> redirect;

    // 遍历所有基本块，找出需要重定向的块
    for (auto *block : func->get_blocks()) {
        auto &instructions = block->get_insts();
        if (instructions.size() > 1) continue;

        if (instructions.empty()) {
            // 空块，重定向到下一个块
            auto blocks_list = func->get_blocks();
            auto block_iter = std::find(blocks_list.begin(), blocks_list.end(), block);
            if (block_iter == blocks_list.end() || std::next(block_iter) == blocks_list.end()) continue;

            auto *next_block = *std::next(block_iter);
            redirect[block] = next_block;
        } else {
            // 只有一个指令的块，检查是否为无条件跳转
            auto *inst = instructions.front();
            if (inst->get_type() == backend::riscv::RVInstType::J) {
                // 获取目标标签
                auto operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
                // 查找目标块
                for (auto *target_block : func->get_blocks()) {
                    if (target_block->get_name() == label->name()) {
                        redirect[block] = target_block;
                        break;
                    }
                }
            }
        }
    }

    if (redirect.empty()) return false;

    // 确定最终的重定向方向
    for (auto &[block, target] : redirect) {
        // logger::INFO(block->get_name(), " ", target->get_name());
        auto *cur = target;
        while (redirect.count(cur)) {
            redirect[block] = redirect[cur];
            // logger::INFO(block->get_name(), " ", redirect[block]->get_name());
            cur = redirect[cur];
        }
    }

    bool modified = false;

    // 更新所有跳转指令的目标
    for (auto *block : func->get_blocks()) {
        for (auto *inst : block->get_insts()) {
            if (inst->get_type() == backend::riscv::RVInstType::J) {
                auto operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
                // 查找当前目标块
                for (auto *target_block : func->get_blocks()) {
                    if (target_block->get_name() == label->name()) {
                        if (redirect.count(target_block)) {
                            // 创建新的标签并替换
                            auto *new_label = backend::riscv::RVLabel::create(redirect[target_block]->get_name());
                            inst->replace_target_label(new_label);
                            modified = true;
                        }
                        break;
                    }
                }
            } else if (inst->is_branch_ins()) {
                auto operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);
                // 查找当前目标块
                for (auto *target_block : func->get_blocks()) {
                    if (target_block->get_name() == label->name()) {
                        if (redirect.count(target_block)) {
                            // 创建新的标签并替换
                            auto *new_label = backend::riscv::RVLabel::create(redirect[target_block]->get_name());
                            // 使用新的replace_target_label方法替换目标标签
                            inst->replace_target_label(new_label);
                            modified = true;
                        }
                        break;
                    }
                }
            }
        }
    }

    // 删除所有空块
    func->remove_empty_blocks();

    // if (modified) {
    //     logger::INFO("redirect_goto: true");
    // }
    return modified;
}

bool SimplifyCFG::remove_unused_labels(backend::riscv::RVFunction *func) {
    // logger::INFO("remove_unused_labels");
    std::unordered_set<backend::riscv::RVBasicBlock *> used_labels;
    std::queue<backend::riscv::RVBasicBlock *> q;

    // 从第一个块开始
    auto blocks_list = func->get_blocks();
    if (blocks_list.empty()) return false;

    auto *first_block = blocks_list.front();
    q.push(first_block);
    used_labels.insert(first_block);

    while (!q.empty()) {
        auto *block = q.front();
        q.pop();

        if (block->get_insts().empty()) {
            // 空块，检查空块的下一个块
            auto block_iter = std::find(blocks_list.begin(), blocks_list.end(), block);
            if (block_iter == blocks_list.end() || std::next(block_iter) == blocks_list.end()) continue;

            auto *next_block = *std::next(block_iter);
            if (used_labels.count(next_block) == 0) {
                q.push(next_block);
                used_labels.insert(next_block);
            }
        } else {
            // 检查所有指令
            for (auto *inst : block->get_insts()) {
                if (inst->is_branch_ins()) {
                    auto operands = inst->get_operands();
                    auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);
                    // 查找目标块
                    for (auto *target_block : func->get_blocks()) {
                        if (target_block->get_name() == label->name()) {
                            if (used_labels.count(target_block) == 0) {
                                q.push(target_block);
                                used_labels.insert(target_block);
                            }
                            break;
                        }
                    }
                } else if (inst->get_type() == backend::riscv::RVInstType::J) {
                    auto operands = inst->get_operands();
                    auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
                    // 查找目标块
                    for (auto *target_block : func->get_blocks()) {
                        if (target_block->get_name() == label->name()) {
                            if (used_labels.count(target_block) == 0) {
                                q.push(target_block);
                                used_labels.insert(target_block);
                            }
                            break;
                        }
                    }
                }
            }

            // 检查最后一个指令
            auto *last_inst = block->get_insts().back();
            if (last_inst->get_type() != backend::riscv::RVInstType::J &&
                last_inst->get_type() != backend::riscv::RVInstType::JR) {
                // 不是无条件跳转，检查下一个块
                auto block_iter = std::find(blocks_list.begin(), blocks_list.end(), block);
                if (block_iter != blocks_list.end() && std::next(block_iter) != blocks_list.end()) {
                    auto *next_block = *std::next(block_iter);
                    if (used_labels.count(next_block) == 0) {
                        q.push(next_block);
                        used_labels.insert(next_block);
                    }
                }
            }
        }
    }

    // 删除未使用的标签
    int original_size = func->get_blocks().size();

    // 创建要删除的块列表
    std::vector<backend::riscv::RVBasicBlock *> blocks_to_remove;
    for (auto *block : func->get_blocks()) {
        if (used_labels.count(block) == 0) {
            blocks_to_remove.push_back(block);
        }
    }

    // 删除未使用的块
    for (auto *block : blocks_to_remove) {
        func->remove_block(block);
    }

    bool modified = original_size != func->get_blocks().size();

    // if (modified) {
    //     logger::INFO("remove_unused_labels: true");
    // }
    return modified;
}

bool SimplifyCFG::conditional_to_unconditional(backend::riscv::RVFunction *func) {
    // logger::INFO("conditional_to_unconditional");
    bool modified = false;
    auto blocks_list = func->get_blocks();

    for (auto it = blocks_list.begin(); it != blocks_list.end(); ++it) {
        auto *block = *it;
        auto next_it = std::next(it);
        if (next_it == blocks_list.end()) break;

        auto *next_block = *next_it;
        auto &instructions = block->get_insts();

        if (instructions.empty()) continue;

        auto *last_inst = instructions.back();
        if (last_inst->is_branch_ins()) {
            auto operands = last_inst->get_operands();
            auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);
            if (label->name() == next_block->get_name()) {
                // 条件跳转的目标是下一个块，转换为无条件跳转

                // 添加无条件跳转指令
                auto *jump_label = backend::riscv::RVLabel::create(next_block->get_name());
                auto *jump_inst = backend::riscv::RVJ::create(jump_label);
                last_inst->replace_self(jump_inst);

                modified = true;
            }
        }
    }

    // if (modified) {
    //     logger::INFO("conditional_to_unconditional: true");
    // }
    return modified;
}

// Short Forward Branch Optimization
bool SimplifyCFG::sfb_opt(backend::riscv::RVFunction *func) {
    // logger::INFO("sfb_opt");
    bool modified = false;
    auto blocks_list = func->get_blocks();

    for (auto it = blocks_list.begin(); it != blocks_list.end(); ++it) {
        auto *block = *it;
        auto next_it = std::next(it);
        if (next_it == blocks_list.end()) continue;

        auto *next_block = *next_it;
        auto &instructions = block->get_insts();

        auto *terminator = instructions.back();
        if (!terminator->is_branch_ins()) continue;

        // 获取分支目标
        auto operands = terminator->get_operands();
        auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);

        // 查找目标块
        backend::riscv::RVBasicBlock *target_block = nullptr;
        for (auto *func_block : func->get_blocks()) {
            if (func_block->get_name() == label->name()) {
                target_block = func_block;
                break;
            }
        }

        if (!target_block || target_block->get_insts().size() != 2) continue;

        // targetBlock 是一个短块

        // 检查目标块的最后一条指令
        auto *back_inst = target_block->get_insts().back();
        if (back_inst->get_type() != backend::riscv::RVInstType::J) continue;

        auto back_operands = back_inst->get_operands();
        auto *back_label = dynamic_cast<backend::riscv::RVLabel *>(back_operands[0]);

        // 查找无条件跳转的目标
        backend::riscv::RVBasicBlock *next_target_block = nullptr;
        for (auto *func_block : func->get_blocks()) {
            if (func_block->get_name() == back_label->name()) {
                next_target_block = func_block;
                break;
            }
        }

        if (next_target_block != next_block) continue;

        // 反转条件跳转
        terminator->inverse();
        // 更新目标块
        auto *new_label = backend::riscv::RVLabel::create(next_target_block->get_name());
        // 使用replace_target_label方法替换目标标签
        terminator->replace_target_label(new_label);

        // 移动指令
        auto *need_move = target_block->get_insts().front();
        auto *cloned_inst = need_move->clone();
        block->add_inst(cloned_inst);

        modified = true;
    }

    // if (modified) {
    //     logger::INFO("sfb_opt: true");
    // }
    return modified;
}

bool SimplifyCFG::reorder_branch(backend::riscv::RVFunction *func) {
    // logger::INFO("reorder_branch");
    bool modified = false;
    auto blocks_list = func->get_blocks();

    for (auto it = blocks_list.begin(); it != blocks_list.end(); ++it) {
        auto *block = *it;
        auto next_it = std::next(it);
        if (next_it == blocks_list.end()) break;

        auto *next_block = *next_it;
        auto &instructions = block->get_insts();

        if (instructions.size() < 2) continue;

        auto *jump = instructions.back();
        auto *branch = *std::next(instructions.rbegin());

        if (branch->is_branch_ins() && jump->get_type() == backend::riscv::RVInstType::J) {
            auto branch_operands = branch->get_operands();
            auto jump_operands = jump->get_operands();

            auto *branch_label = dynamic_cast<backend::riscv::RVLabel *>(branch_operands[2]);
            auto *jump_label = dynamic_cast<backend::riscv::RVLabel *>(jump_operands[0]);

            // 检查分支目标是否为下一个块
            if (branch_label->name() == next_block->get_name()) {
                // 反转分支条件
                branch->inverse();

                // 更新分支目标
                auto *new_label = backend::riscv::RVLabel::create(jump_label->name());
                // 使用replace_target_label方法替换分支指令的目标标签
                branch->replace_target_label(new_label);

                // 删除跳转指令
                jump->delete_self();

                modified = true;
            }
        }
    }

    // if (modified) {
    //     logger::INFO("reorder_branch: true");
    // }
    return modified;
}

bool SimplifyCFG::block_link_opt(backend::riscv::RVFunction *func) {
    // logger::INFO("block_link_opt");
    bool modify = false;
    auto blocks_list = func->get_blocks();

    for (auto it = blocks_list.begin(); it != blocks_list.end(); ++it) {
        auto *block = *it;
        auto next_it = std::next(it);
        if (next_it == blocks_list.end()) break;

        auto *next_block = *next_it;
        auto &instructions = block->get_insts();

        if (instructions.empty()) continue;

        auto *last_inst = instructions.back();
        if (last_inst->get_type() == backend::riscv::RVInstType::J) {
            auto operands = last_inst->get_operands();
            auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
            if (label && label->name() == next_block->get_name()) {
                // 跳转目标是下一个块，删除跳转指令
                last_inst->delete_self();
                modify = true;
            }
        }
    }
    // if (modify) {
    //     logger::INFO("block_link_opt: true");
    // }
    return modify;
}

void SimplifyCFG::simplify_cfg(backend::riscv::RVFunction *func) {
    while (true) {
        bool modified = false;
        modified |= block_link_opt(func);
        modified |= redirect_goto(func);
        modified |= reorder_branch(func);
        // modified |= Peephole::generic_peephole_opt(func);  // 暂时注释掉
        modified |= conditional_to_unconditional(func);
        modified |= redirect_goto(func);
        // modified |= Peephole::generic_peephole_opt(func);  // 暂时注释掉
        modified |= remove_unused_labels(func);
        modified |= sfb_opt(func);

        if (!modified) {
            return;
        }
    }
}

}  // namespace backend::opt::cfg_opt
