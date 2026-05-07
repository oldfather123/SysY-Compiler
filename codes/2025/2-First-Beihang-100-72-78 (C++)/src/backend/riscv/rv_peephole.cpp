#include "rv_peephole.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <set>

#include "log.hpp"
#include "rv_function.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

PeepHole::PeepHole(RVModule *module) : riscv_module(module), current_function(nullptr) {}

void PeepHole::after_ra_run() {
    bool is_finished = false;
    while (!is_finished) {
        is_finished = peephole();
    }
    // j_next_optimize();
}

void PeepHole::before_ra_run() {
    loop_peephole();
    delete_useless_def();
    delete_useless_label();
    branch_optimize();
}

bool PeepHole::peephole() {
    bool is_finished = true;

    for (auto &[func_name, function] : riscv_module->get_functions()) {
        if (function->is_external()) continue;
        // 设置当前正在处理的函数
        current_function = function;

        // 初始化当前函数的活跃信息映射
        function->liveness_analysis();
        for (auto *block : function->get_blocks()) {
            live_info_map[block] = block->get_live_info();
        }

        // 正序遍历blocks
        for (auto *block : function->get_blocks()) {
            is_finished &= mv_sd_optimize(block);
            auto &instructions = block->get_insts();

            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end();) {
                bool should_increment = true;

                // 检查并执行各种优化，如果任何优化删除了指令，则不要递增迭代器
                should_increment &= move_same_reg_optimize(inst_iter, instructions);
                should_increment &= addi_optimize(inst_iter, instructions);
                should_increment &= store_load_optimize(inst_iter, instructions);
                should_increment &= remove_useless_stack_fixer(inst_iter, instructions);
                should_increment &= merge_addi_sp(inst_iter, instructions, block);

                if (should_increment) {
                    ++inst_iter;
                } else {
                    is_finished = false;
                }
            }
        }

        // 倒序遍历blocks - 使用正向迭代器避免失效问题
        auto &blocks_list = function->get_blocks();
        for (int i = blocks_list.size() - 1; i >= 0; --i) {
            auto block_iter = std::next(blocks_list.begin(), i);
            auto *block = *block_iter;
            auto &instructions = block->get_insts();

            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end();) {
                if (move_useless_mv(inst_iter, instructions)) {
                    ++inst_iter;
                }
            }
            is_finished &= merge_block(function, block);
        }
    }
    return is_finished;
}

void PeepHole::j_next_optimize() {
    for (auto &[func_name, function] : riscv_module->get_functions()) {
        auto &blocks = function->get_blocks();
        auto block_iter = blocks.begin();

        while (block_iter != blocks.end()) {
            auto *current_block = *block_iter;
            auto &instructions = current_block->get_insts();

            if (!instructions.empty()) {
                auto last_inst = instructions.back();
                if (last_inst->get_type() == RVInstType::J) {
                    auto *target = dynamic_cast<RVLabel *>(last_inst->get_operands()[0]);
                    // 检查下一个block是否存在且名称匹配
                    auto next_block_iter = std::next(block_iter);
                    if (next_block_iter != blocks.end()) {
                        auto *next_block = *next_block_iter;
                        if (next_block->get_name() == target->to_string()) {
                            // 删除多余的j指令
                            last_inst->delete_self();
                        }
                    }
                }
            }

            ++block_iter;
        }
    }
}

void PeepHole::useless_ls_remove(backend::riscv::RVBasicBlock *block) {
    // 遍历基本块中的所有指令
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return;
    }

    for (auto iter = std::next(instructions.begin()); iter != instructions.end();) {
        backend::riscv::RVInstruction *current_instr = *iter;
        backend::riscv::RVInstruction *before_instr = *std::prev(iter);

        // 检查前一条指令和当前指令都是内存访问指令（load或store）
        if (before_instr->is_memory_ins() && current_instr->is_memory_ins()) {
            // 获取操作数
            const auto &before_operands = before_instr->get_operands();
            const auto &current_operands = current_instr->get_operands();

            auto *before_val = before_instr->get_memory_val();
            auto *current_val = current_instr->get_memory_val();
            auto *before_base = before_instr->get_memory_base();
            auto *current_base = current_instr->get_memory_base();

            // 比较addr（地址偏移）
            auto *before_offset = before_operands[2];
            auto *current_offset = current_operands[2];

            bool offset_match = true;
            if (before_offset->get_kind() == backend::riscv::RVOperand::Kind::IMM &&
                current_offset->get_kind() == backend::riscv::RVOperand::Kind::IMM) {
                auto *before_imm = dynamic_cast<backend::riscv::RVImmediate *>(before_offset);
                auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_offset);
                offset_match = (before_imm->value() == current_imm->value());
            } else {
                // 其他情况，比较字符串表示
                offset_match = (before_offset->to_string() == current_offset->to_string());
            }

            if (offset_match && before_val == current_val && before_base == current_base) {
                // 从指令列表中移除并更新迭代器
                iter = (*iter)->delete_self();
                continue;  // 跳过++iter，因为erase已经更新了迭代器
            }
        }

        ++iter;
    }
}

bool PeepHole::move_same_reg_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions) {
    bool finished = true;
    if (inst_iter != instructions.end() && (*inst_iter)->is_move_ins()) {
        auto *def_reg = (*inst_iter)->get_def();
        auto uses = (*inst_iter)->get_uses();
        if (def_reg == uses[0]) {
            inst_iter = (*inst_iter)->delete_self();
            finished = false;
        }
    }
    return finished;
}

bool PeepHole::addi_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions) {
    bool finished = true;
    if (inst_iter != instructions.end() && (*inst_iter)->is_addi_ins()) {
        auto operands = (*inst_iter)->get_operands();
        auto *imm = dynamic_cast<RVImmediate *>(operands[2]);
        if (imm->value() == 0) {
            auto *def_reg = (*inst_iter)->get_def();
            if (def_reg == operands[1]) {
                inst_iter = (*inst_iter)->delete_self();
                finished = false;
            } else {
                auto *new_mv = RVMv::create(def_reg, operands[1]);
                inst_iter = (*inst_iter)->replace_self(new_mv);
                finished = false;
            }
        }
    }
    return finished;
}

bool PeepHole::store_load_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions) {
    bool finished = true;
    if (inst_iter != instructions.end() && inst_iter != instructions.begin()) {
        auto prev_iter = std::prev(inst_iter);

        bool is_store_load = false;
        RVInstruction *store_inst = nullptr;
        RVInstruction *load_inst = nullptr;

        if ((*inst_iter)->is_load_ins() && (*prev_iter)->is_store_ins()) {
            // Check for LD/SD or LW/SW pairs
            if (((*inst_iter)->get_type() == RVInstType::LD && (*prev_iter)->get_type() == RVInstType::SD) ||
                ((*inst_iter)->get_type() == RVInstType::LW && (*prev_iter)->get_type() == RVInstType::SW)) {
                is_store_load = true;
                store_inst = *prev_iter;
                load_inst = *inst_iter;
            }
        }

        if (is_store_load) {
            auto store_ops = store_inst->get_operands();
            auto load_ops = load_inst->get_operands();

            if (store_ops[0] == load_ops[1] && store_ops[2] == load_ops[2]) {
                auto *new_mv = RVMv::create(load_inst->get_def(), store_ops[1]);
                (*prev_iter)->delete_self();
                inst_iter = (*inst_iter)->replace_self(new_mv);
                finished = false;
            }
        }
    }
    return finished;
}

bool PeepHole::remove_useless_stack_fixer(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions) {
    bool finished = true;

    // 检查当前指令是否为LI指令
    if (inst_iter != instructions.end() && (*inst_iter)->get_type() == RVInstType::LI) {
        auto operands = (*inst_iter)->get_operands();
        auto *stack_fixer = dynamic_cast<RVStackFixer *>(operands[1]);
        if (stack_fixer) {
            int aligned_offset = stack_fixer->get_aligned_offset();
            auto *def_reg = (*inst_iter)->get_def();

            // 检查下一个指令
            auto next_iter = std::next(inst_iter);
            if (next_iter != instructions.end()) {
                auto *next_inst = *next_iter;

                // 检查是否为Binary指令（ADD/SUB/ADDW/SUBW）
                if (next_inst->is_add_sub_ins()) {
                    auto next_operands = next_inst->get_operands();
                    // 检查是否为SP寄存器的特殊情况
                    if (next_inst->get_def() == reg_info::get_sp()) {
                        if (aligned_offset >= 2048 || aligned_offset < -2048) {
                            return true;
                        }

                        // 处理SP寄存器的SUB操作
                        if (next_inst->get_type() == RVInstType::SUB) {
                            // 查找对应的ADD指令
                            auto node_iter = std::next(next_iter);
                            // assert(dynamic_cast<RVCall*>(*node_iter) != nullptr);
                            while (node_iter != instructions.end() && ((*node_iter)->get_type() != RVInstType::ADD ||
                                                                       (*node_iter)->get_def() != reg_info::get_sp())) {
                                ++node_iter;
                            }

                            if (node_iter != instructions.end() && (*node_iter)->get_type() == RVInstType::ADD &&
                                (*node_iter)->get_def() == reg_info::get_sp()) {
                                // 创建新的ADDI指令
                                auto *new_addi1 = RVAddi::create(
                                    reg_info::get_sp(), reg_info::get_sp(), RVImmediate::create(-aligned_offset));
                                auto *new_addi2 = RVAddi::create(
                                    reg_info::get_sp(), reg_info::get_sp(), RVImmediate::create(aligned_offset));

                                inst_iter = (*inst_iter)->delete_self();
                                inst_iter = (*inst_iter)->replace_self(new_addi1);

                                (*node_iter)->replace_self(new_addi2);
                                return false;
                            }
                        }
                    }

                    // 处理ADD指令
                    if (next_inst->get_type() == RVInstType::ADD) {
                        if (next_operands[1] == def_reg && aligned_offset < 2048 && aligned_offset >= -2048) {
                            finished = false;
                            auto *new_addi = RVAddi::create(
                                next_inst->get_def(), next_operands[2], RVImmediate::create(aligned_offset));

                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addi);
                        } else if (next_operands[2] == def_reg && aligned_offset < 2048 && aligned_offset >= -2048) {
                            finished = false;
                            auto *new_addi = RVAddi::create(
                                next_inst->get_def(), next_operands[1], RVImmediate::create(aligned_offset));
                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addi);
                        }
                    }
                    // 处理SUB指令
                    else if (next_inst->get_type() == RVInstType::SUB) {
                        if (next_operands[2] == def_reg && aligned_offset <= 2048 && aligned_offset > -2048) {
                            finished = false;
                            auto *new_addi = RVAddi::create(
                                next_inst->get_def(), next_operands[1], RVImmediate::create(-aligned_offset));
                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addi);
                        }
                    }
                    // 处理SUBW指令
                    else if (next_inst->get_type() == RVInstType::SUBW) {
                        if (next_operands[2] == def_reg && aligned_offset <= 2048 && aligned_offset > -2048) {
                            finished = false;
                            auto *new_addiw = RVAddiw::create(
                                next_inst->get_def(), next_operands[1], RVImmediate::create(-aligned_offset));
                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addiw);
                        }
                    }
                    // 处理ADDW指令
                    else if (next_inst->get_type() == RVInstType::ADDW) {
                        if (next_operands[1] == def_reg && aligned_offset < 2048 && aligned_offset >= -2048) {
                            finished = false;
                            auto *new_addiw = RVAddiw::create(
                                next_inst->get_def(), next_operands[2], RVImmediate::create(aligned_offset));
                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addiw);
                        } else if (next_operands[2] == def_reg && aligned_offset < 2048 && aligned_offset >= -2048) {
                            finished = false;
                            auto *new_addiw = RVAddiw::create(
                                next_inst->get_def(), next_operands[1], RVImmediate::create(aligned_offset));
                            inst_iter = (*inst_iter)->delete_self();
                            inst_iter = (*inst_iter)->replace_self(new_addiw);
                        }
                    }
                }
            }
        }
    }

    return finished;
}

// 合并条件前提是初步生成的后端指令中，sw\lw等指令读写内存时，只有和sp相关的指令有偏移，其余没有
bool PeepHole::merge_addi_sp(RVBasicBlock::InstIterator &inst_iter,
                             RVBasicBlock::InstList &instructions,
                             RVBasicBlock *block) {
    bool finished = true;

    // 检查当前指令是否为addi或addiw
    if (inst_iter == instructions.end()) {
        return finished;
    }

    auto *current_inst = *inst_iter;
    if (current_inst->get_type() != RVInstType::ADDI && current_inst->get_type() != RVInstType::ADDIW) {
        return finished;
    }

    // 检查定义寄存器不是sp寄存器（寄存器2）
    auto *def_reg = current_inst->get_def();
    if (!def_reg || def_reg == reg_info::get_cpu_reg(2)) {
        return finished;
    }

    // 检查下一条指令
    auto next_iter = std::next(inst_iter);
    if (next_iter == instructions.end()) {
        return finished;
    }

    auto *next_inst = *next_iter;
    auto next_type = next_inst->get_type();

    // 检查是否可以删除addi指令
    if (!can_delete_addi(block, next_iter, def_reg)) {
        return finished;
    }

    // 检查定义寄存器不等于源寄存器（避免自赋值）
    auto operands = current_inst->get_operands();
    if (def_reg == operands[1]) {
        return finished;
    }

    RVInstruction *new_inst = nullptr;
    auto next_operands = next_inst->get_operands();

    bool flag0 = def_reg == next_operands[0];
    bool flag1 = def_reg == next_operands[1];
    // 处理存储指令
    if (next_type == RVInstType::SW && flag0) {
        new_inst = RVSw::create(operands[1], next_operands[1], operands[2]);
    } else if (next_type == RVInstType::SD && flag0) {
        new_inst = RVSd::create(operands[1], next_operands[1], operands[2]);
    } else if (next_type == RVInstType::FSW && flag0) {
        new_inst = RVFsw::create(operands[1], next_operands[1], operands[2]);
    } else if (next_type == RVInstType::FSD && flag0) {
        new_inst = RVFsd::create(operands[1], next_operands[1], operands[2]);
    }
    // 处理加载指令
    else if (next_type == RVInstType::LW && flag1) {
        new_inst = RVLw::create(next_operands[0], operands[1], operands[2]);
    } else if (next_type == RVInstType::LD && flag1) {
        new_inst = RVLd::create(next_operands[0], operands[1], operands[2]);
    } else if (next_type == RVInstType::FLW && flag1) {
        new_inst = RVFlw::create(next_operands[0], operands[1], operands[2]);
    } else if (next_type == RVInstType::FLD && flag1) {
        new_inst = RVFld::create(next_operands[0], operands[1], operands[2]);
    }

    // 如果创建了新指令，则替换原来的指令
    if (new_inst != nullptr) {
        finished = false;
        inst_iter = (*inst_iter)->delete_self();
        inst_iter = (*inst_iter)->replace_self(new_inst);
    }

    return finished;
}

bool PeepHole::move_useless_mv(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions) {
    bool finished = true;
    if (inst_iter != instructions.end() && (*inst_iter)->is_move_ins()) {
        auto *def_reg = (*inst_iter)->get_def();
        auto operands = (*inst_iter)->get_operands();
        if (def_reg == operands[1]) {
            inst_iter = (*inst_iter)->delete_self();
            finished = false;
        } else if (inst_iter != instructions.begin()) {
            auto prev_iter = std::prev(inst_iter);
            if ((*prev_iter)->is_move_ins() && (*prev_iter)->get_def() == def_reg) {
                (*prev_iter)->delete_self();
                finished = false;
            }
        }
    }
    return finished;
}

// 合并block
bool PeepHole::merge_block(RVFunction *function, RVBasicBlock *block) {
    bool finished = true;

    // 获取当前block的指令列表
    auto &instructions = block->get_insts();
    if (instructions.empty()) {
        return finished;
    }

    // 获取最后一条指令
    RVInstruction *last_inst = instructions.back();

    // 检查最后一条指令是否是跳转指令 (J指令)
    if (last_inst->get_type() == RVInstType::J) {
        // 获取跳转目标block
        auto operands = last_inst->get_operands();
        if (operands.empty()) {
            return finished;
        }

        // 获取跳转目标标签
        auto *target_label = dynamic_cast<RVLabel *>(operands[0]);
        if (!target_label) {
            return finished;
        }

        // 查找目标block
        RVBasicBlock *target_block = nullptr;
        for (auto *func_block : function->get_blocks()) {
            if (func_block->get_name() == target_label->to_string()) {
                target_block = func_block;
                break;
            }
        }

        if (!target_block) {
            return finished;
        }

        // 检查target_block是否紧跟在当前block之后
        bool is_adjacent = false;
        auto blocks_list = function->get_blocks();
        auto block_iter = std::find(blocks_list.begin(), blocks_list.end(), block);
        if (block_iter != blocks_list.end() && std::next(block_iter) != blocks_list.end()) {
            if (*std::next(block_iter) == target_block) {
                is_adjacent = true;
            }
        }

        if (!is_adjacent) {
            return finished;
        }

        // 检查全局是否还有其他地方跳转到target_block
        bool has_other_jumps = false;
        for (auto *func_block : function->get_blocks()) {
            if (func_block == block) {
                continue;  // 跳过当前block
            }

            auto &block_instructions = func_block->get_insts();
            // 从后往前检查跳转指令
            for (auto inst_iter = block_instructions.rbegin(); inst_iter != block_instructions.rend(); ++inst_iter) {
                RVInstruction *inst = *inst_iter;

                // 检查J指令
                if (inst->get_type() == RVInstType::J) {
                    auto inst_operands = inst->get_operands();
                    if (!inst_operands.empty()) {
                        auto *inst_label = dynamic_cast<RVLabel *>(inst_operands[0]);
                        if (inst_label && inst_label->to_string() == target_label->to_string()) {
                            has_other_jumps = true;
                            break;
                        }
                    }
                }
                // 检查分支指令 (BEQ, BNE, BLT, BGE)
                else if (inst->is_branch_ins()) {
                    auto inst_operands = inst->get_operands();
                    auto *inst_label = dynamic_cast<RVLabel *>(inst_operands[2]);
                    if (inst_label && inst_label->to_string() == target_label->to_string()) {
                        has_other_jumps = true;
                        break;
                    }
                }
            }

            if (has_other_jumps) {
                break;
            }
        }

        // 如果没有其他地方跳转到target_block，则合并两个block
        if (!has_other_jumps) {
            // 删除最后的跳转指令
            instructions.back()->delete_self();

            // target_block的所有后继肯定已经被当前block包含，只需从当前block的后继中移除target_block
            block->remove_succ(target_block);

            // 将target_block的指令添加到当前block
            for (auto *target_inst : target_block->get_insts()) {
                block->add_inst(target_inst);
            }

            auto &target_instructions = target_block->get_insts();

            // 清空target_block的指令列表（避免重复删除）
            target_instructions.clear();

            // 从函数中移除target_block
            function->remove_block(target_block);

            finished = false;
        }
    }

    return finished;
}

void PeepHole::loop_peephole() {
    // logger::INFO("Loop peephole started...");
    for (auto &[func_name, function] : riscv_module->get_functions()) {
        // 正序遍历blocks
        // TODO livInfoMap 和 influencedReg 处理
        current_function = function;
        for (auto *block : function->get_blocks()) {
            // logger::INFO(block->get_name());
            auto &instructions = block->get_insts();

            // 消除多余li指令
            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end(); ++inst_iter) {
                if ((*inst_iter)->get_type() == RVInstType::LI) {
                    // logger::INFO("remove li: ", (*inst_iter)->to_string());
                    auto *def_reg = (*inst_iter)->get_def();
                    if (auto *vir_reg = dynamic_cast<RVVirReg *>(def_reg)) {
                        auto operands = (*inst_iter)->get_operands();
                        if (auto *imm = dynamic_cast<RVImmediate *>(operands[1])) {
                            // 查找后续相同的li指令
                            for (auto next_iter = std::next(inst_iter); next_iter != instructions.end(); ++next_iter) {
                                if ((*next_iter)->get_type() == RVInstType::LI &&
                                    dynamic_cast<RVVirReg *>((*next_iter)->get_def())) {
                                    auto next_ops = (*next_iter)->get_operands();
                                    auto *next_imm = dynamic_cast<RVImmediate *>(next_ops[1]);
                                    if (next_imm && next_imm->value() == imm->value()) {
                                        // 检查是否有其他使用，如果有则跳过
                                        auto *next_def = (*next_iter)->get_def();
                                        bool other_use = false;
                                        for (auto *use_inst : next_def->get_graph_uses()) {
                                            for (auto *use : use_inst->get_uses()) {
                                                if (use == next_def) {
                                                    other_use = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (other_use) {
                                            continue;
                                        }
                                        logger::INFO("loop_peephole");
                                        logger::INFO((*next_iter)->to_string());
                                        // 先复制使用列表，避免迭代器失效
                                        auto next_def_uses = next_def->get_graph_uses();
                                        for (auto *use_inst : next_def_uses) {
                                            use_inst->replace_regs(next_def, vir_reg);
                                        }
                                        (*next_iter)->delete_self();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 消除重复的二元运算指令
            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end(); ++inst_iter) {
                if ((*inst_iter)->is_binary_ins()) {
                    // logger::INFO("remove same binary ins: ", (*inst_iter)->to_string());
                    auto *def_reg = (*inst_iter)->get_def();
                    if (auto *vir_reg = dynamic_cast<RVVirReg *>(def_reg)) {
                        auto operands = (*inst_iter)->get_operands();
                        // 查找后续相同的二元运算指令
                        for (auto next_iter = std::next(inst_iter); next_iter != instructions.end(); ++next_iter) {
                            if (((*next_iter)->get_type() == (*inst_iter)->get_type()) &&
                                dynamic_cast<RVVirReg *>((*next_iter)->get_def())) {
                                auto next_ops = (*next_iter)->get_operands();
                                // 检查操作数是否相同（考虑交换律）
                                if ((next_ops[1] == operands[1] && next_ops[2] == operands[2]) ||
                                    (next_ops[1] == operands[2] && next_ops[2] == operands[1])) {
                                    // 检查是否有其他使用，如果有则跳过
                                    auto *next_def = (*next_iter)->get_def();
                                    bool other_use = false;
                                    for (auto *use_inst : next_def->get_graph_uses()) {
                                        for (auto *use : use_inst->get_uses()) {
                                            if (use == next_def) {
                                                other_use = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (other_use) {
                                        continue;
                                    }
                                    logger::INFO("loop_peephole");
                                    logger::INFO((*next_iter)->to_string());
                                    // 先复制使用列表，避免迭代器失效
                                    auto next_def_uses = next_def->get_graph_uses();
                                    for (auto *use_inst : next_def_uses) {
                                        use_inst->replace_regs(next_def, vir_reg);
                                    }
                                    (*next_iter)->delete_self();
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void PeepHole::delete_useless_def() {
    // logger::INFO("Deleting useless def...");
    for (auto &[func_name, function] : riscv_module->get_functions()) {
        for (auto *block : function->get_blocks()) {
            auto &instructions = block->get_insts();

            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end();) {
                auto *def_reg = (*inst_iter)->get_def();
                bool should_increment = true;
                if (def_reg && dynamic_cast<RVVirReg *>(def_reg)) {
                    if (def_reg->get_graph_uses().empty()) {
                        inst_iter = (*inst_iter)->delete_self();
                        should_increment = false;
                    }
                }
                if (should_increment) {
                    ++inst_iter;
                }
            }
        }
    }
}

void PeepHole::delete_useless_label() {
    // TODO: 实现无用标签的删除
}

// 刚好原来的Br指令都是与zero进行比较，而且都对应bne
void PeepHole::branch_optimize() {
    // logger::INFO("Branching optimization...");
    // 遍历所有函数
    for (const auto &[func_name, func] : riscv_module->get_functions()) {
        current_function = func;

        // 遍历函数中的所有基本块
        for (auto *block : func->get_blocks()) {
            auto &instructions = block->get_insts();

            // 遍历基本块中的所有指令
            for (auto inst_iter = instructions.begin(); inst_iter != instructions.end(); ++inst_iter) {
                auto *instr = *inst_iter;

                // 处理SLT指令优化
                if (instr->get_type() == RVInstType::SLT) {
                    bool can_optimize = true;

                    // 检查该指令定义寄存器的所有使用者
                    auto *def_reg = instr->get_def();
                    if (!def_reg) continue;

                    for (auto *user_inst : def_reg->get_graph_uses()) {
                        if (user_inst == instr) continue;

                        // 如果不是分支指令或者不能移动，则不能优化
                        if (!user_inst->is_branch_ins() ||
                            !check_between_icmp_and_br(inst_iter,
                                                       std::find(instructions.begin(), instructions.end(), user_inst),
                                                       instr->get_operands()[0],
                                                       instr->get_operands()[1],
                                                       instructions)) {
                            can_optimize = false;
                            break;
                        }
                    }

                    if (can_optimize) {
                        // logger::INFO("branch_optimize");
                        // logger::INFO(instr->to_string());
                        // 处理所有使用该寄存器的分支指令
                        std::vector<std::pair<RVInstruction *, RVInstruction *>> replacements;
                        for (auto *user_inst : def_reg->get_graph_uses()) {
                            if (user_inst == instr) continue;

                            auto *branch_inst = user_inst;
                            auto *op1 = instr->get_operands()[1];
                            auto *op2 = instr->get_operands()[2];
                            auto *label = branch_inst->get_operands()[2];

                            RVInstruction *new_branch = nullptr;

                            if (branch_inst->get_type() == RVInstType::BNE) {
                                new_branch = RVBlt::create(op1, op2, label);
                            } else if (branch_inst->get_type() == RVInstType::BEQ) {
                                new_branch = RVBge::create(op1, op2, label);
                            }

                            if (new_branch) {
                                replacements.emplace_back(branch_inst, new_branch);
                            }
                        }

                        // 在循环外执行替换
                        for (const auto &[old_inst, new_inst] : replacements) {
                            old_inst->replace_self(new_inst);
                        }

                        // 删除原始的SLT指令
                        inst_iter = (*inst_iter)->delete_self();
                    }
                }

                // 处理SLTI指令优化
                else if (instr->get_type() == RVInstType::SLTI) {
                    // 检查前一条指令是否为减法指令
                    if (inst_iter == instructions.begin()) continue;
                    auto prev_iter = std::prev(inst_iter);
                    auto *prev_instr = *prev_iter;

                    if (prev_instr->get_type() == RVInstType::SUBW) {
                        auto *op1 = prev_instr->get_operands()[1];
                        auto *op2 = prev_instr->get_operands()[2];

                        bool can_optimize = true;
                        auto *def_reg = instr->get_def();
                        if (!def_reg) continue;

                        for (auto *user_inst : def_reg->get_graph_uses()) {
                            if (user_inst == instr || user_inst == prev_instr) continue;

                            if (!user_inst->is_branch_ins() ||
                                !check_between_icmp_and_br(
                                    inst_iter,
                                    std::find(instructions.begin(), instructions.end(), user_inst),
                                    op1,
                                    op2,
                                    instructions)) {
                                can_optimize = false;
                                break;
                            }
                        }

                        if (can_optimize) {
                            // logger::INFO("branch_optimize");
                            // logger::INFO((*prev_iter)->to_string());
                            // logger::INFO((*inst_iter)->to_string());

                            std::vector<std::pair<RVInstruction *, RVInstruction *>> replacements;
                            for (auto *user_inst : def_reg->get_graph_uses()) {
                                if (user_inst == instr || user_inst == prev_instr) continue;

                                auto *branch_inst = user_inst;
                                auto *label = branch_inst->get_operands()[2];

                                RVInstruction *new_branch = nullptr;

                                if (branch_inst->get_type() == RVInstType::BNE) {
                                    new_branch = RVBle::create(op1, op2, label);
                                } else if (branch_inst->get_type() == RVInstType::BEQ) {
                                    new_branch = RVBgt::create(op1, op2, label);
                                }

                                if (new_branch) {
                                    replacements.emplace_back(branch_inst, new_branch);
                                }
                            }

                            // 在循环外执行替换
                            for (const auto &[old_inst, new_inst] : replacements) {
                                old_inst->replace_self(new_inst);
                            }

                            // 删除SLTI和SUB指令
                            (*prev_iter)->delete_self();
                            inst_iter = (*inst_iter)->delete_self();
                        }
                    }
                }

                // 处理SLTIU指令优化
                else if (instr->get_type() == RVInstType::SLTIU) {
                    // 检查前一条指令是否为减法指令
                    if (inst_iter == instructions.begin()) continue;
                    auto prev_iter = std::prev(inst_iter);
                    auto *prev_instr = *prev_iter;

                    if (prev_instr->get_type() == RVInstType::SUBW) {
                        auto *op1 = prev_instr->get_operands()[1];
                        auto *op2 = prev_instr->get_operands()[2];

                        bool can_optimize = true;
                        auto *def_reg = instr->get_def();
                        if (!def_reg) continue;

                        for (auto *user_inst : def_reg->get_graph_uses()) {
                            if (user_inst == instr || user_inst == prev_instr) continue;

                            if (!user_inst->is_branch_ins() ||
                                !check_between_icmp_and_br(
                                    inst_iter,
                                    std::find(instructions.begin(), instructions.end(), user_inst),
                                    op1,
                                    op2,
                                    instructions)) {
                                can_optimize = false;
                                break;
                            }
                        }

                        if (can_optimize) {
                            // logger::INFO("branch_optimize");
                            // logger::INFO((*prev_iter)->to_string());
                            // logger::INFO((*inst_iter)->to_string());

                            std::vector<std::pair<RVInstruction *, RVInstruction *>> replacements;
                            for (auto *user_inst : def_reg->get_graph_uses()) {
                                if (user_inst == instr || user_inst == prev_instr) continue;

                                auto *branch_inst = user_inst;
                                auto *label = branch_inst->get_operands()[2];

                                RVInstruction *new_branch = nullptr;

                                if (branch_inst->get_type() == RVInstType::BNE) {
                                    new_branch = RVBeq::create(op1, op2, label);
                                } else if (branch_inst->get_type() == RVInstType::BEQ) {
                                    new_branch = RVBne::create(op1, op2, label);
                                }

                                if (new_branch) {
                                    replacements.emplace_back(branch_inst, new_branch);
                                }
                            }

                            // 在循环外执行替换
                            for (const auto &[old_inst, new_inst] : replacements) {
                                old_inst->replace_self(new_inst);
                            }

                            // 删除SLTIU和SUB指令
                            (*prev_iter)->delete_self();
                            inst_iter = (*inst_iter)->delete_self();
                        }
                    }
                }

                // 处理SLTU指令优化
                else if (instr->get_type() == RVInstType::SLTU) {
                    // 检查前一条指令是否为减法指令
                    if (inst_iter == instructions.begin()) continue;
                    auto prev_iter = std::prev(inst_iter);
                    auto *prev_instr = *prev_iter;

                    if (prev_instr->get_type() == RVInstType::SUBW) {
                        auto *op1 = prev_instr->get_operands()[1];
                        auto *op2 = prev_instr->get_operands()[2];

                        bool can_optimize = true;
                        auto *def_reg = instr->get_def();
                        if (!def_reg) continue;

                        for (auto *user_inst : def_reg->get_graph_uses()) {
                            if (user_inst == instr || user_inst == prev_instr) continue;

                            if (!user_inst->is_branch_ins() ||
                                !check_between_icmp_and_br(
                                    inst_iter,
                                    std::find(instructions.begin(), instructions.end(), user_inst),
                                    op1,
                                    op2,
                                    instructions)) {
                                can_optimize = false;
                                break;
                            }
                        }

                        if (can_optimize) {
                            // logger::INFO("branch_optimize");
                            // logger::INFO((*prev_iter)->to_string());
                            // logger::INFO((*inst_iter)->to_string());

                            std::vector<std::pair<RVInstruction *, RVInstruction *>> replacements;
                            for (auto *user_inst : def_reg->get_graph_uses()) {
                                if (user_inst == instr || user_inst == prev_instr) continue;

                                auto *branch_inst = user_inst;
                                auto *label = branch_inst->get_operands()[2];

                                RVInstruction *new_branch = nullptr;

                                if (branch_inst->get_type() == RVInstType::BNE) {
                                    new_branch = RVBne::create(op1, op2, label);
                                } else if (branch_inst->get_type() == RVInstType::BEQ) {
                                    new_branch = RVBeq::create(op1, op2, label);
                                }

                                if (new_branch) {
                                    replacements.emplace_back(branch_inst, new_branch);
                                }
                            }

                            // 在循环外执行替换
                            for (const auto &[old_inst, new_inst] : replacements) {
                                old_inst->replace_self(new_inst);
                            }

                            // 删除SLTU和SUB指令
                            (*prev_iter)->delete_self();
                            inst_iter = (*inst_iter)->delete_self();
                        }
                    }
                }
            }
        }
    }
}

bool PeepHole::can_delete_addi(RVBasicBlock *block, RVBasicBlock::InstIterator inst_iter, RVReg *reg) {
    // 优先对当前块进行遍历
    auto *current_inst = *inst_iter;
    if (current_inst->is_load_ins()) {
        if (current_inst->get_def() == reg) {
            return true;
        }
    }

    // 遍历当前指令之后的指令
    auto after_iter = inst_iter;
    ++after_iter;
    while (after_iter != block->get_insts().end()) {
        auto *after_inst = *after_iter;

        // 检查操作数中是否使用了该寄存器
        for (auto *use : after_inst->get_uses()) {
            if (use == reg) {
                return false;
            }
        }

        // 检查是否定义了该寄存器
        auto def_regs = get_def_phi_reg(after_inst);
        for (auto *def_reg : def_regs) {
            if (def_reg == reg) {
                return true;
            }
        }

        ++after_iter;
    }

    // 如果寄存器不在受影响的寄存器集合中，检查live_out
    if (influenced_reg.find(reg) == influenced_reg.end()) {
        return block->get_live_info().live_out.find(reg) == block->get_live_info().live_out.end();
    }

    // 对一级子孙进行遍历
    for (auto *succ_block : block->get_succs()) {
        auto after_iter = succ_block->get_insts().begin();
        while (after_iter != succ_block->get_insts().end()) {
            auto *after_inst = *after_iter;

            // 检查操作数中是否使用了该寄存器
            for (auto *use : after_inst->get_uses()) {
                if (use == reg) {
                    return false;
                }
            }

            // 检查是否定义了该寄存器
            auto def_regs = get_def_phi_reg(after_inst);
            bool found_def = false;
            for (auto *def_reg : def_regs) {
                if (def_reg == reg) {
                    found_def = true;
                    break;
                }
            }
            if (found_def) {
                break;
            }

            ++after_iter;
        }
    }

    // 重新进行活跃性分析，只分析当前函数
    current_function->liveness_analysis();
    for (auto *block : current_function->get_blocks()) {
        live_info_map[block] = block->get_live_info();
    }

    // TODO: influenced_reg.clear();

    return block->get_live_info().live_out.find(reg) == block->get_live_info().live_out.end();
}

std::set<RVPhyReg *> PeepHole::get_def_phi_reg(RVInstruction *inst) {
    std::set<RVPhyReg *> ret;

    if (inst->get_type() != RVInstType::CALL) {
        if (auto *def_reg = inst->get_def()) {
            // 如果是物理寄存器，直接添加
            if (auto *phy_reg = dynamic_cast<RVPhyReg *>(def_reg)) {
                ret.insert(phy_reg);
            }
        }
    } else {
        // CALL指令会修改多个寄存器
        std::vector<int> cpu_regs = {1, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};
        std::vector<int> fpu_regs = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};

        for (int reg_id : cpu_regs) {
            ret.insert(reg_info::get_cpu_reg(reg_id));
        }
        for (int reg_id : fpu_regs) {
            ret.insert(reg_info::get_fpu_reg(reg_id));
        }
    }

    return ret;
}

bool PeepHole::check_between_icmp_and_br(RVBasicBlock::InstIterator cmp_iter,
                                         RVBasicBlock::InstIterator br_iter,
                                         RVOperand *op1,
                                         RVOperand *op2,
                                         RVBasicBlock::InstList &instructions) {
    // 在同一个块内，且中间无其他节点 defReg 为cmp_iter的Op
    auto ins_iter = cmp_iter;
    while (ins_iter != instructions.end()) {
        ++ins_iter;
        if (ins_iter == br_iter) {
            return true;
        } else {
            auto *inst = *ins_iter;
            auto *def_reg = inst->get_def();
            if (def_reg == op1 || def_reg == op2) {
                return false;
            }
        }
    }
    return false;
}

bool PeepHole::mv_sd_optimize(backend::riscv::RVBasicBlock *block) {
    // logger::INFO("mv_sd_optimize: ", block->get_name());
    bool is_finished = true;
    // 遍历基本块中的所有指令
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return is_finished;
    }

    for (auto iter = std::next(instructions.begin()); iter != instructions.end();) {
        backend::riscv::RVInstruction *current_instr = *iter;
        backend::riscv::RVInstruction *before_instr = *std::prev(iter);

        // logger::INFO("cur: ", current_instr->to_string());
        // logger::INFO("before: ", before_instr->to_string());
        if (before_instr->is_move_ins()) {
            if (current_instr->is_store_ins()) {
                const auto &mv_operands = before_instr->get_operands();
                auto *mv_rd = dynamic_cast<backend::riscv::RVReg *>(mv_operands[0]);
                auto *mv_rs = dynamic_cast<backend::riscv::RVReg *>(mv_operands[1]);

                const auto &sd_operands = current_instr->get_operands();
                auto *sd_rs = dynamic_cast<backend::riscv::RVReg *>(sd_operands[1]);
                auto *sd_base = dynamic_cast<backend::riscv::RVReg *>(sd_operands[0]);
                auto *sd_offset = sd_operands[2];

                if (mv_rd == sd_rs) {
                    backend::riscv::RVInstruction *new_store = nullptr;

                    // 根据存储指令的类型创建相应的新指令
                    switch (current_instr->get_type()) {
                        case backend::riscv::RVInstType::SB:
                            new_store = backend::riscv::RVSb::create(sd_base, mv_rs, sd_offset);
                            break;
                        case backend::riscv::RVInstType::SH:
                            new_store = backend::riscv::RVSh::create(sd_base, mv_rs, sd_offset);
                            break;
                        case backend::riscv::RVInstType::SW:
                            new_store = backend::riscv::RVSw::create(sd_base, mv_rs, sd_offset);
                            break;
                        case backend::riscv::RVInstType::SD:
                            new_store = backend::riscv::RVSd::create(sd_base, mv_rs, sd_offset);
                            break;
                        case backend::riscv::RVInstType::FSW:
                            new_store = backend::riscv::RVFsw::create(sd_base, mv_rs, sd_offset);
                            break;
                        case backend::riscv::RVInstType::FSD:
                            new_store = backend::riscv::RVFsd::create(sd_base, mv_rs, sd_offset);
                            break;
                        default:
                            break;
                    }

                    if (new_store) {
                        is_finished = false;
                        if (before_instr->get_def()->get_graph_uses().size() == 2) {
                            before_instr->delete_self();
                        }
                        iter = (*iter)->replace_self(new_store);
                        // 无论是否替换都要++iter
                    }
                }
            }
        }

        ++iter;
    }
    return is_finished;
}

}  // namespace backend::riscv
