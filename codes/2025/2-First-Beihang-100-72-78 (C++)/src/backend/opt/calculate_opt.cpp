#include "backend/opt/calculate_opt.hpp"

#include <algorithm>
#include <climits>
#include <iterator>
#include <queue>
#include <unordered_map>
#include <vector>

#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_operand.hpp"
#include "backend/riscv/rv_reg_info.hpp"
#include "log.hpp"

namespace backend::opt {

void CalculateOpt::run_before_ra(backend::riscv::RVModule *module) {
    for (int i = 0; i < 1; i++) {
        // 第一轮：局部值复用只能跑一遍，影响if-combine2
        for (const auto &[name, function] : module->get_functions()) {
            // 跳过外部函数（blocks为空的函数）
            if (function->is_external()) continue;

            // 对每个基本块执行优化
            for (auto *block : function->get_blocks()) {
                useless_ls_remove(block);
                // li_0_to_mv_zero(block);
                pre_ra_const_value_reuse(block);
                pre_ra_const_imm_cal_reuse(block);
                pre_ra_const_pointer_reuse(block);
                pre_ra_addi_sp_reuse(block);
                add_zero_to_mv(block);
                // addi_ls_to_ls_offset(block);
                one_to_zero_beq(block);
                // seqz_branch_reverse(block);
            }
        }
    }

    for (int i = 0; i < 1; i++) {
        // 第二轮：额外的优化只能跑一遍，影响if-combine3
        for (const auto &[name, function] : module->get_functions()) {
            // 跳过外部函数
            if (function->is_external()) continue;

            for (auto *block : function->get_blocks()) {
                // icmp_branch_to_branch(block);
                sra_sll_to_and(block);
                // lsw_to_lsd(block);
                li_to_lui_or_addiw(block);
                // slliw_add_to_shadd(block);
            }
        }
    }

    bool is_finished = false;
    while (!is_finished) {
        is_finished = true;
        for (const auto &[name, function] : module->get_functions()) {
            // 跳过外部函数
            if (function->is_external()) continue;

            for (auto *block : function->get_blocks()) {
                is_finished &= li_mv_to_li(block);
                is_finished &= sw_lw_to_sw_mv(block);
                is_finished &= remove_redundant_sw(block);
                is_finished &= mv_instruction_optimization(block);
                is_finished &= li_zero_optimization(block);
                is_finished &= merge_consecutive_slliw(block);
            }

            auto &blocks = function->get_blocks();

            for (auto it = blocks.begin(); it != blocks.end();) {
                bool should_increment = true;
                should_increment &= optimize_li_block_inlining(function, it);
                if (should_increment) {
                    ++it;
                } else {
                    is_finished = false;
                }
            }

            for (auto it = blocks.begin(); it != blocks.end();) {
                bool should_increment = true;
                should_increment &= optimize_single_block_loop(function, it);
                if (should_increment) {
                    ++it;
                } else {
                    is_finished = false;
                }
            }
        }
    }
}

void CalculateOpt::run_aft_bin(backend::riscv::RVModule *module) {
    // TODO: 在块内联后运行
}

void CalculateOpt::seqz_branch_reverse(backend::riscv::RVBasicBlock *block) {
    // TODO: seqz分支反转
}

bool CalculateOpt::sra_sll_to_and(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 将连续的sraiw和slliw指令优化为andi指令
    auto &instructions = block->get_insts();

    // 遍历指令列表，检查相邻的指令对
    for (auto iter = instructions.begin(); iter != instructions.end();) {
        // 检查是否有下一条指令
        auto next_iter = std::next(iter);
        if (next_iter == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *iter;
        backend::riscv::RVInstruction *next_instr = *next_iter;
        backend::riscv::RVInstruction *new_andi = nullptr;

        // 检查当前指令是否为SRAIW
        if (current_instr->get_type() == backend::riscv::RVInstType::SRAIW) {
            // 检查下一条指令是否为SLLIW
            if (next_instr->get_type() == backend::riscv::RVInstType::SLLIW) {
                const auto &sraiw_operands = current_instr->get_operands();
                const auto &slliw_operands = next_instr->get_operands();

                // 获取SRAIW的操作数
                auto *sraiw_rd = dynamic_cast<backend::riscv::RVReg *>(sraiw_operands[0]);
                auto *sraiw_rs1 = dynamic_cast<backend::riscv::RVReg *>(sraiw_operands[1]);
                auto *sraiw_imm = dynamic_cast<backend::riscv::RVImmediate *>(sraiw_operands[2]);

                // 获取SLLIW的操作数
                auto *slliw_rd = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[0]);
                auto *slliw_rs1 = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[1]);
                auto *slliw_imm = dynamic_cast<backend::riscv::RVImmediate *>(slliw_operands[2]);

                // 检查立即数值是否相同
                if (sraiw_imm && slliw_imm && sraiw_imm->value() == slliw_imm->value()) {
                    // 检查SRAIW的目标寄存器是否与SLLIW的源寄存器相同
                    if (sraiw_rd && slliw_rs1 && sraiw_rd == slliw_rs1) {
                        // 计算ANDI的立即数值：-(1 << shift_amount)
                        int shift_amount = static_cast<int>(slliw_imm->value());
                        int andi_value = -(1 << shift_amount);

                        // 检查立即数是否在有效范围内（-2047到2047）
                        if (andi_value >= -2048) {
                            // 创建新的ANDI指令
                            new_andi = backend::riscv::RVAndi::create(
                                slliw_rd,                                        // 目标寄存器
                                sraiw_rs1,                                       // 源寄存器
                                backend::riscv::RVImmediate::create(andi_value)  // 立即数
                            );
                        }
                    }
                }
            }
        }
        if (new_andi) {
            iter = (*iter)->delete_self();
            iter = (*iter)->replace_self(new_andi);
            is_finished = false;
        } else {
            ++iter;
        }
    }
    return is_finished;
}

void CalculateOpt::addi_ls_to_ls_offset(backend::riscv::RVBasicBlock *block) {
    // logger::INFO("addi_ls_to_ls_offset");
    // 将addi+ls匹配成直接写的偏移
    auto &instructions = block->get_insts();
    for (auto iter = instructions.begin(); iter != instructions.end();) {
        backend::riscv::RVInstruction *current_instr = *iter;

        // 检查是否为ADDI指令
        if (current_instr->get_type() == backend::riscv::RVInstType::ADDI) {
            bool can_remove = true;

            // 获取ADDI指令的操作数
            const auto &operands = current_instr->get_operands();

            auto *rd = dynamic_cast<backend::riscv::RVReg *>(operands[0]);
            auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(operands[1]);
            auto *rs2 = dynamic_cast<backend::riscv::RVImmediate *>(operands[2]);

            // 获取使用rd寄存器的所有指令
            const auto &uses = rd->get_graph_uses();

            for (auto *inst : uses) {
                // 检查是否为内存访问指令
                if (inst->is_memory_ins()) {
                    auto *ls_base = inst->get_memory_base();
                    auto *ls_val = inst->get_memory_val();

                    // 检查内存指令的基址寄存器是否为ADDI的目标寄存器
                    if (ls_base == rd) {
                        // 检查偏移量是否为0
                        const auto &ls_operands = inst->get_operands();
                        auto *offset = ls_operands[2];
                        if (auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(offset)) {
                            if (imm->value() == 0 && ls_val != rd) {
                                // 可以合并：将ADDI的立即数作为新的偏移量
                                long new_offset = rs2->value();

                                // logger::INFO("addi_ls_to_ls_offset");
                                // logger::INFO(inst->to_string());
                                // 更新内存指令的基址寄存器为ADDI的源寄存器
                                inst->replace_regs(rd, rs1);
                                // logger::INFO(inst->to_string());

                                // 创建新的立即数作为偏移量
                                auto *new_imm = backend::riscv::RVImmediate::create(new_offset);

                                // 替换偏移量操作数（第三个操作数）
                                // 替换为新的立即数
                                const_cast<std::vector<backend::riscv::RVOperand *> &>(inst->get_operands())[2] =
                                    new_imm;

                                // 暂时跳过详细的对齐检查

                            } else if (ls_val == rd) {
                                // 如果内存指令的值寄存器就是ADDI的目标寄存器，不能删除
                                can_remove = false;
                            }
                        }
                    }
                } else {
                    // 如果不是内存指令，检查是否使用了ADDI的目标寄存器
                    auto uses_regs = inst->get_uses();
                    for (auto *use_reg : uses_regs) {
                        if (use_reg == rd) {
                            can_remove = false;
                            break;
                        }
                    }
                }
            }

            if (can_remove) {
                // logger::INFO((*iter)->to_string());
                iter = (*iter)->delete_self();
            } else {
                ++iter;
            }
        } else {
            ++iter;
        }
    }
}

bool CalculateOpt::useless_ls_remove(backend::riscv::RVBasicBlock *block) {
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
                is_finished = false;
                continue;
            }
        }

        ++iter;
    }
    return is_finished;
}

bool CalculateOpt::one_to_zero_beq(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 遍历指令列表，检查相邻的指令对
    auto &instructions = block->get_insts();
    for (auto it = instructions.begin(); it != instructions.end();) {
        // 检查是否有下一条指令
        auto next_it = std::next(it);
        if (next_it == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *it;
        backend::riscv::RVInstruction *next_instr = *next_it;
        backend::riscv::RVInstruction *new_branch = nullptr;

        // 检查当前指令是否为LI指令
        if (current_instr->get_type() == backend::riscv::RVInstType::LI) {
            const auto &li_operands = current_instr->get_operands();
            auto *rd = dynamic_cast<backend::riscv::RVReg *>(li_operands[0]);
            auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(li_operands[1]);

            // 检查下一个指令是否为分支指令
            if (next_instr->is_branch_ins()) {
                const auto &branch_operands = next_instr->get_operands();
                auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(branch_operands[0]);
                auto *rs2 = dynamic_cast<backend::riscv::RVReg *>(branch_operands[1]);
                auto *target = dynamic_cast<backend::riscv::RVLabel *>(branch_operands[2]);

                // 获取zero寄存器
                auto *zero = backend::riscv::reg_info::get_zero();

                // 检查各种优化条件并直接替换
                if (imm->value() == 1) {
                    if (rs1 == rd && next_instr->get_type() == backend::riscv::RVInstType::BGT) {
                        // li rd, 1; bgt rd, rs2, target -> bge zero, rs2, target
                        new_branch = backend::riscv::RVBge::create(zero, rs2, target);
                    } else if (rs2 == rd && next_instr->get_type() == backend::riscv::RVInstType::BLT) {
                        // li rd, 1; blt rs1, rd, target -> ble rs1, zero, target
                        new_branch = backend::riscv::RVBle::create(rs1, zero, target);
                    }
                } else if (imm->value() == -1) {
                    if (rs2 == rd && next_instr->get_type() == backend::riscv::RVInstType::BGT) {
                        // li rd, -1; bgt rs1, rd, target -> bge rs1, zero, target
                        new_branch = backend::riscv::RVBge::create(rs1, zero, target);
                    } else if (rs1 == rd && next_instr->get_type() == backend::riscv::RVInstType::BLT) {
                        // li rd, -1; blt rd, rs2, target -> ble zero, rs2, target
                        new_branch = backend::riscv::RVBle::create(zero, rs2, target);
                    }
                }
            }
        }
        if (new_branch) {
            is_finished = false;
            // logger::INFO("one_to_zero_beq", block->get_name());
            // logger::INFO(current_instr->to_string());
            // logger::INFO(next_instr->to_string());
            it = (*it)->delete_self();
            it = (*it)->replace_self(new_branch);
        } else {
            ++it;
        }
    }
    return is_finished;
}

bool CalculateOpt::add_zero_to_mv(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("add_zero_to_mv");
    // 遍历基本块中的所有指令
    auto &instructions = block->get_insts();

    // 如果指令列表为空，直接返回
    if (instructions.empty()) {
        return is_finished;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *inst = *it;

        // 检查是否是ADD或ADDW指令
        if (inst->get_type() == backend::riscv::RVInstType::ADD ||
            inst->get_type() == backend::riscv::RVInstType::ADDW) {
            const auto &operands = inst->get_operands();
            auto *rd = operands[0];
            auto *rs1 = operands[1];
            auto *rs2 = operands[2];

            // 检查rs2是否是零寄存器
            if (rs2 == backend::riscv::reg_info::get_zero()) {
                // logger::INFO(inst->to_string());
                // 创建mv指令：mv rd, rs1
                auto *mv_inst = backend::riscv::RVMv::create(rd, rs1);
                it = (*it)->replace_self(mv_inst);
                is_finished = false;
                continue;
            }

            // 检查rs1是否是零寄存器
            if (rs1 == backend::riscv::reg_info::get_zero()) {
                // 创建mv指令：mv rd, rs2
                auto *mv_inst = backend::riscv::RVMv::create(rd, rs2);
                it = (*it)->replace_self(mv_inst);
                is_finished = false;
                continue;
            }
        }

        ++it;
    }
    return is_finished;
}

bool CalculateOpt::match_eq(backend::riscv::RVInstruction *now,
                            backend::riscv::RVInstruction *next,
                            backend::riscv::RVInstruction *far_next) {
    // TODO: 匹配eq模式
    return false;
}

bool CalculateOpt::match_ne(backend::riscv::RVInstruction *now,
                            backend::riscv::RVInstruction *next,
                            backend::riscv::RVInstruction *far_next) {
    // TODO: 匹配ne模式
    return false;
}

bool CalculateOpt::match_sge_and_sle(backend::riscv::RVInstruction *now,
                                     backend::riscv::RVInstruction *next,
                                     backend::riscv::RVInstruction *far_next) {
    // TODO: 匹配sge和sle模式
    return false;
}

bool CalculateOpt::match_slt_and_sgt(backend::riscv::RVInstruction *now, backend::riscv::RVInstruction *next) {
    // TODO: 匹配slt和sgt模式
    return false;
}

bool CalculateOpt::match_far_next(backend::riscv::RVInstruction *now,
                                  backend::riscv::RVInstruction *next,
                                  backend::riscv::RVInstruction *far_next) {
    // TODO: 匹配far_next模式
    return false;
}

void CalculateOpt::icmp_branch_to_branch(backend::riscv::RVBasicBlock *block) {
    // TODO: 将icmp和branch合并
}

bool CalculateOpt::pre_ra_const_value_reuse(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("pre_ra_const_value_reuse: ", block->get_name());
    const int range = 16;                                                   // 生存周期范围
    std::unordered_map<long, std::pair<backend::riscv::RVReg *, int>> map;  // 值 -> (寄存器, 生存周期)
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *instr = *it;
        if (dynamic_cast<riscv::RVPhyReg *>(instr->get_def())) {
            ++it;
            continue;
        }

        // 检查指令是否有定义操作数，如果有且不是Li指令，则删除所有重定义的寄存器
        if (instr->get_type() != backend::riscv::RVInstType::LI) {
            if (instr->get_type() == backend::riscv::RVInstType::CALL) {
                for (auto map_it = map.begin(); map_it != map.end();) {
                    if (riscv::reg_info::is_callee_saved_reg(map_it->second.first)) {
                        map_it = map.erase(map_it);
                    } else {
                        ++map_it;
                    }
                }

            } else {
                auto *def_reg = instr->get_def();
                if (def_reg) {
                    // logger::INFO("remove: ", instr->to_string());
                    for (auto map_it = map.begin(); map_it != map.end();) {
                        if (map_it->second.first == def_reg) {
                            // logger::INFO("ok");
                            map_it = map.erase(map_it);
                        } else {
                            ++map_it;
                        }
                    }
                }
            }
        }

        // 检查是否是Li指令
        if (instr->get_type() == backend::riscv::RVInstType::LI) {
            // 获取Li指令的立即数值
            const auto &operands = instr->get_operands();
            auto *imm_operand = operands[1];
            if (auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(imm_operand)) {
                long value = imm->value();
                auto *def_reg = instr->get_def();

                // 检查是否已经记录了这个值
                auto map_it = map.find(value);
                if (map_it != map.end()) {
                    // logger::INFO(def_reg->to_string());
                    // 如果前面有记录这个值，那么就将这个li给删掉，然后将后面的所有使用这个的寄存器都换掉
                    auto *now_reg = map_it->second.first;
                    if (now_reg != def_reg) {
                        // 先复制使用列表，避免迭代器失效
                        auto uses = def_reg->get_graph_uses();
                        for (auto *use_instr : uses) {
                            use_instr->replace_regs(def_reg, now_reg);
                        }
                    }
                    map_it->second.second = range;  // 刷新生存周期
                    // logger::INFO(instr->to_string());

                    it = (*it)->delete_self();
                    is_finished = false;
                    continue;  // 跳过++it，因为erase已经更新了迭代器
                } else {
                    // 删除所有使用def_reg的记录
                    for (auto map_it2 = map.begin(); map_it2 != map.end();) {
                        if (map_it2->second.first == def_reg) {
                            map_it2 = map.erase(map_it2);
                        } else {
                            ++map_it2;
                        }
                    }
                    // 添加新的值记录
                    // logger::INFO("add: ",instr->to_string());
                    map[value] = std::make_pair(def_reg, range);
                }
            }
        }

        // 更新所有记录的生存周期
        for (auto map_it = map.begin(); map_it != map.end();) {
            map_it->second.second--;
            if (map_it->second.second == 0) {
                map_it = map.erase(map_it);
            } else {
                ++map_it;
            }
        }

        ++it;
    }
    return is_finished;
}

bool CalculateOpt::li_to_lui_or_addiw(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *inst = *it;
        backend::riscv::RVInstruction *new_inst = nullptr;
        // 检查是否是Li指令
        if (inst->get_type() == backend::riscv::RVInstType::LI) {
            const auto &operands = inst->get_operands();
            // Li指令格式：li rd, imm，操作数为[rd, imm]
            auto *imm_operand = operands[1];
            if (auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(imm_operand)) {
                long long value = imm->value();

                if (value > 0 && value <= INT_MAX && ((value >> 12) << 12) == value) {
                    // 创建新的Lui指令：lui rd, (value >> 12)
                    auto *def_reg = inst->get_def();
                    auto *new_imm = backend::riscv::RVImmediate::create(value >> 12);
                    new_inst = backend::riscv::RVLui::create(def_reg, new_imm);
                }
                // else if (value < 2048 && value >= -2048) {
                //     // addiw 比 li 快？
                //     auto *def_reg = inst->get_def();
                //     new_inst = backend::riscv::RVAddiw::create(def_reg, riscv::reg_info::get_zero(), imm);
                // }
            }
        }
        if (new_inst) {
            is_finished = false;
            it = (*it)->replace_self(new_inst);
        } else {
            ++it;
        }
    }
    return is_finished;
}

bool CalculateOpt::pre_ra_const_imm_cal_reuse(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("pre_ra_const_imm_cal_reuse: ", block->get_name());
    std::queue<std::pair<backend::riscv::RVReg *, long long>> queue;
    const int window_size = 4;
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *instr = *it;
        if (dynamic_cast<riscv::RVPhyReg *>(instr->get_def())) {
            ++it;
            continue;
        }

        if (instr->get_type() == backend::riscv::RVInstType::LI) {
            const auto &operands = instr->get_operands();
            auto *imm_operand = operands[1];
            if (auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(imm_operand)) {
                long long imm_val = imm->value();
                bool flag = imm_val >= 2048 || imm_val < -2048;

                auto *def_reg = instr->get_def();
                backend::riscv::RVInstruction *new_instr = nullptr;
                std::queue<std::pair<backend::riscv::RVReg *, long long>> temp_queue = std::queue(queue);

                while (!temp_queue.empty()) {
                    auto pair = temp_queue.front();
                    temp_queue.pop();
                    backend::riscv::RVReg *existing_reg = pair.first;
                    long long existing_val = pair.second;

                    if (imm_val == existing_val) {
                        // mv rd, existing_reg
                        new_instr = backend::riscv::RVMv::create(def_reg, existing_reg);
                        break;
                    } else if (flag && imm_val - existing_val <= 2047 && imm_val - existing_val >= -2048) {
                        // addiw rd, existing_reg, imm_val - existing_val
                        auto *new_imm = backend::riscv::RVImmediate::create(imm_val - existing_val);
                        new_instr = backend::riscv::RVAddiw::create(def_reg, existing_reg, new_imm);
                        break;
                    } else if (flag && -imm_val == existing_val) {
                        // sub rd, zero, existing_reg
                        new_instr = backend::riscv::RVSub::create(def_reg, riscv::reg_info::get_zero(), existing_reg);
                        break;
                    } else if (~imm_val == existing_val) {
                        // xori rd, existing_reg, -1
                        auto *new_imm = backend::riscv::RVImmediate::create(-1);
                        new_instr = backend::riscv::RVXori::create(def_reg, existing_reg, new_imm);
                        break;
                    }
                    // else if (imm_val == 3 * existing_val) {
                    //     assert(false);
                    //     // sh1add rd, existing_reg, existing_reg
                    //     new_instr = backend::riscv::RVSh1add::create(def_reg, existing_reg, existing_reg);
                    //     break;
                    // } else if (imm_val == 5 * existing_val) {
                    //     assert(false);
                    //     // sh2add rd, existing_reg, existing_reg
                    //     new_instr = backend::riscv::RVSh2add::create(def_reg, existing_reg, existing_reg);
                    //     break;
                    // } else if (imm_val == 9 * existing_val) {
                    //     assert(false);
                    //     // sh3add rd, existing_reg, existing_reg
                    //     new_instr = backend::riscv::RVSh3add::create(def_reg, existing_reg, existing_reg);
                    //     break;
                    // }
                    else if (flag) {
                        int max_k = 8;
                        for (int k = 1; k <= max_k; k++) {
                            if (imm_val == existing_val << k) {
                                // slliw rd, existing_reg, k
                                auto *new_imm = backend::riscv::RVImmediate::create(k);
                                new_instr = backend::riscv::RVSlliw::create(def_reg, existing_reg, new_imm);
                                break;
                            } else if (imm_val == existing_val >> k) {
                                // srliw rd, existing_reg, k
                                auto *new_imm = backend::riscv::RVImmediate::create(k);
                                new_instr = backend::riscv::RVSraiw::create(def_reg, existing_reg, new_imm);
                                break;
                            }
                        }
                    }
                }

                if (new_instr) {
                    // logger::INFO("inst: ", (*it)->to_string());
                    // logger::INFO("new_instr: ", new_instr->to_string());
                    // 统一替换操作
                    it = (*it)->replace_self(new_instr);
                    is_finished = false;
                } else {
                    queue.push(std::make_pair(def_reg, imm_val));
                    while (queue.size() > window_size) {
                        queue.pop();
                    }
                    ++it;
                }
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    return is_finished;
}

bool CalculateOpt::pre_ra_const_pointer_reuse(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("pre_ra_const_pointer_reuse");
    const int range = 16;
    std::unordered_map<std::string, std::pair<backend::riscv::RVReg *, int>> map;
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *instr = *it;
        if (dynamic_cast<riscv::RVPhyReg *>(instr->get_def())) {
            ++it;
            continue;
        }

        // 检查指令是否有定义操作数，如果有且不是Li指令，则删除所有重定义的寄存器
        if (instr->get_type() != backend::riscv::RVInstType::LA) {
            if (instr->get_type() == backend::riscv::RVInstType::CALL) {
                for (auto map_it = map.begin(); map_it != map.end();) {
                    if (riscv::reg_info::is_callee_saved_reg(map_it->second.first)) {
                        map_it = map.erase(map_it);
                    } else {
                        ++map_it;
                    }
                }

            } else {
                auto *def_reg = instr->get_def();
                if (def_reg) {
                    // logger::INFO("remove: ", instr->to_string());
                    for (auto map_it = map.begin(); map_it != map.end();) {
                        if (map_it->second.first == def_reg) {
                            // logger::INFO("ok");
                            map_it = map.erase(map_it);
                        } else {
                            ++map_it;
                        }
                    }
                }
            }
        }

        // 检查是否为La指令
        if (instr->get_type() == backend::riscv::RVInstType::LA) {
            // logger::INFO(instr->to_string());
            // 获取La指令的操作数：rd和label
            auto &operands = instr->get_operands();
            auto *rd_reg = dynamic_cast<backend::riscv::RVReg *>(operands[0]);
            auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[1]);

            // 检查是否已有相同标签的映射
            auto map_it = map.find(label->name());
            if (map_it != map.end()) {
                // 如果前面有记录这个值，那么就将这个la给删掉，然后将后面的所有使用这个的寄存器都换掉
                auto *now_reg = map_it->second.first;
                if (now_reg != rd_reg) {
                    // 先复制使用列表，避免迭代器失效
                    auto uses = rd_reg->get_graph_uses();
                    for (auto *use_instr : uses) {
                        use_instr->replace_regs(rd_reg, now_reg);
                    }
                }
                map_it->second.second = range;  // 刷新生存周期

                // assert(false);
                // 删除重复的La指令
                // logger::INFO("deleted instr: ", instr->to_string());
                it = (*it)->delete_self();
                is_finished = false;
                continue;  // 跳过++it，因为erase已经更新了迭代器
            } else {
                // 删除所有使用rd_reg的映射
                for (auto map_it2 = map.begin(); map_it2 != map.end();) {
                    if (map_it2->second.first == rd_reg) {
                        map_it2 = map.erase(map_it2);
                    } else {
                        ++map_it2;
                    }
                }
                // 添加新的映射
                map[label->name()] = std::make_pair(rd_reg, range);
            }
        }

        // 更新生存周期计数并删除生存周期为0的映射
        for (auto map_it = map.begin(); map_it != map.end();) {
            map_it->second.second--;
            if (map_it->second.second == 0) {
                map_it = map.erase(map_it);
            } else {
                ++map_it;
            }
        }

        ++it;
    }
    return is_finished;
}

void CalculateOpt::aft_bin_const_value_reuse(backend::riscv::RVBasicBlock *block) {
    // TODO: 块内联后常量值重用
}

void CalculateOpt::remove_same_mv(backend::riscv::RVBasicBlock *block) {
    // TODO: 删除相同的mv
}

void CalculateOpt::sub_zero_remove(backend::riscv::RVBasicBlock *block) {
    // TODO: 删除减零操作
}

bool CalculateOpt::ls_conflict(
    backend::riscv::RVBasicBlock *block, int i, int j, bool check_in_lw, backend::riscv::RVReg *base, long offset) {
    // TODO: 检查load/store冲突
    return false;
}

void CalculateOpt::lsw_to_lsd(backend::riscv::RVBasicBlock *block) {
    // TODO: lsw转换为lsd
}

void CalculateOpt::mv_copy(backend::riscv::RVReg *bef, backend::riscv::RVReg *aft) {
    // TODO: 复制mv
}

backend::riscv::RVReg *CalculateOpt::li_find(backend::riscv::RVReg *best_reg, long value) {
    // TODO: 查找li
    return nullptr;
}

void CalculateOpt::li_0_to_mv_zero(backend::riscv::RVBasicBlock *block) {
    // logger::INFO("li_0_to_mv_zero");
    // 遍历基本块中的所有指令
    auto &instructions = block->get_insts();

    // 如果指令列表为空，直接返回
    if (instructions.empty()) {
        return;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *inst = *it;

        // 检查是否是LI指令
        if (inst->get_type() == backend::riscv::RVInstType::LI) {
            const auto &operands = inst->get_operands();
            auto *rd = operands[0];
            auto *imm = operands[1];

            // 检查立即数是否为0
            if (auto *immediate = dynamic_cast<backend::riscv::RVImmediate *>(imm)) {
                if (immediate->value() == 0) {
                    // 创建mv指令：mv rd, zero
                    auto *mv_inst = backend::riscv::RVMv::create(rd, backend::riscv::reg_info::get_zero());
                    it = (*it)->replace_self(mv_inst);
                    continue;
                }
            }
        }

        ++it;
    }
}

void CalculateOpt::slliw_add_to_shadd(backend::riscv::RVBasicBlock *block) {
    // 遍历基本块中的所有指令，检查相邻的指令对
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return;
    }

    for (auto iter = instructions.begin(); iter != instructions.end();) {
        // 检查是否有下一条指令
        auto next_iter = std::next(iter);
        if (next_iter == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *iter;
        backend::riscv::RVInstruction *next_instr = *next_iter;
        backend::riscv::RVInstruction *new_shadd = nullptr;

        // 检查当前指令是否为SLLIW指令
        if (current_instr->get_type() == backend::riscv::RVInstType::SLLIW) {
            // 检查下一条指令是否为ADD指令
            if (next_instr->get_type() == backend::riscv::RVInstType::ADD) {
                const auto &slliw_operands = current_instr->get_operands();
                const auto &add_operands = next_instr->get_operands();

                // 获取SLLIW的操作数
                auto *slliw_rd = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[0]);
                auto *slliw_rs1 = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[1]);
                auto *slliw_imm = dynamic_cast<backend::riscv::RVImmediate *>(slliw_operands[2]);

                // 获取ADD的操作数
                auto *add_rd = dynamic_cast<backend::riscv::RVReg *>(add_operands[0]);
                auto *add_rs1 = dynamic_cast<backend::riscv::RVReg *>(add_operands[1]);
                auto *add_rs2 = dynamic_cast<backend::riscv::RVReg *>(add_operands[2]);

                // 检查SLLIW的目标寄存器是否与ADD的源操作数之一相同
                if (slliw_rd && slliw_imm && ((add_rs1 == slliw_rd) || (add_rs2 == slliw_rd))) {
                    // 获取移位量
                    int shift_amount = static_cast<int>(slliw_imm->value());

                    // 根据移位量决定使用哪种SHADD指令
                    if (shift_amount >= 1 && shift_amount <= 3) {
                        backend::riscv::RVReg *shadd_rs1 = nullptr;
                        backend::riscv::RVReg *shadd_rs2 = nullptr;

                        // 确定SHADD指令的操作数
                        if (add_rs1 == slliw_rd) {
                            // ADD rd, slliw_rd, rs2 -> SHADD rd, slliw_rs1, rs2
                            shadd_rs1 = slliw_rs1;
                            shadd_rs2 = add_rs2;
                        } else {
                            // ADD rd, rs1, slliw_rd -> SHADD rd, slliw_rs1, rs1
                            shadd_rs1 = slliw_rs1;
                            shadd_rs2 = add_rs1;
                        }

                        // 根据移位量创建相应的SHADD指令
                        switch (shift_amount) {
                            case 1:
                                new_shadd = backend::riscv::RVSh1add::create(add_rd, shadd_rs1, shadd_rs2);
                                break;
                            case 2:
                                new_shadd = backend::riscv::RVSh2add::create(add_rd, shadd_rs1, shadd_rs2);
                                break;
                            case 3:
                                new_shadd = backend::riscv::RVSh3add::create(add_rd, shadd_rs1, shadd_rs2);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }

        if (new_shadd) {
            // logger::INFO("slliw_add_to_shadd");
            // logger::INFO(current_instr->to_string());
            // logger::INFO(next_instr->to_string());
            // logger::INFO(new_shadd->to_string());
            if (current_instr->get_def()->get_graph_uses().size() == 2) {
                // logger::INFO("deleted: ", current_instr->to_string());
                current_instr->delete_self();
                iter = next_instr->replace_self(new_shadd);
            }
        } else {
            ++iter;
        }
    }
}

// 匹配的是li经过复用后的变体
bool CalculateOpt::li_mv_to_li(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("li_mv_to_li");
    // 遍历基本块中的所有指令
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return is_finished;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        // 检查是否有下一条指令
        auto next_it = std::next(it);
        if (next_it == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *it;
        backend::riscv::RVInstruction *next_instr = *next_it;

        // 检查下一条指令是否为MV指令
        if (next_instr->get_type() == backend::riscv::RVInstType::MV) {
            const auto &mv_operands = next_instr->get_operands();
            auto *mv_rd = dynamic_cast<backend::riscv::RVReg *>(mv_operands[0]);
            auto *mv_rs1 = dynamic_cast<backend::riscv::RVReg *>(mv_operands[1]);

            // 检查当前指令的类型和操作数
            bool can_optimize = false;
            long long imm_value = 0;
            backend::riscv::RVReg *current_rs1 = nullptr;
            backend::riscv::RVReg *current_rs2 = nullptr;

            switch (current_instr->get_type()) {
                case backend::riscv::RVInstType::LI: {
                    // li + mv 组合
                    const auto &li_operands = current_instr->get_operands();
                    auto *li_rd = dynamic_cast<backend::riscv::RVReg *>(li_operands[0]);
                    auto *li_imm = dynamic_cast<backend::riscv::RVImmediate *>(li_operands[1]);

                    if (li_imm && li_rd && mv_rs1 && li_rd == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(li_rd) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &li_rd_uses = li_rd->get_graph_uses();
                        if (li_rd_uses.size() == 2) {
                            can_optimize = true;
                            imm_value = li_imm->value();
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::MV: {
                    // mv + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);

                    if (current_rd_temp && current_rs1_temp && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::ADDIW: {
                    // addiw + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                    auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_operands[2]);

                    if (current_rd_temp && current_rs1_temp && current_imm && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                            imm_value = current_imm->value();
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::SUB: {
                    // sub + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                    auto *current_rs2_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[2]);

                    if (current_rd_temp && current_rs1_temp && current_rs2_temp && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                            current_rs2 = current_rs2_temp;
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::XORI: {
                    // xori + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                    auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_operands[2]);

                    if (current_rd_temp && current_rs1_temp && current_imm && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                            imm_value = current_imm->value();
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::SLLIW: {
                    // slliw + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                    auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_operands[2]);

                    if (current_rd_temp && current_rs1_temp && current_imm && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                            imm_value = current_imm->value();
                        }
                    }
                    break;
                }
                case backend::riscv::RVInstType::SRAIW: {
                    // sraiw + mv 组合
                    const auto &current_operands = current_instr->get_operands();
                    auto *current_rd_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                    auto *current_rs1_temp = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                    auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_operands[2]);

                    if (current_rd_temp && current_rs1_temp && current_imm && current_rd_temp == mv_rs1 &&
                        dynamic_cast<backend::riscv::RVVirReg *>(current_rd_temp) &&
                        dynamic_cast<backend::riscv::RVVirReg *>(mv_rd)) {
                        const auto &current_rd_uses = current_rd_temp->get_graph_uses();
                        if (current_rd_uses.size() == 2) {
                            can_optimize = true;
                            current_rs1 = current_rs1_temp;
                            imm_value = current_imm->value();
                        }
                    }
                    break;
                }
                default:
                    break;
            }

            // 如果可以优化，执行优化
            if (can_optimize) {
                backend::riscv::RVInstruction *new_inst = nullptr;

                // 根据不同的指令类型创建相应的新指令
                switch (current_instr->get_type()) {
                    case backend::riscv::RVInstType::LI:
                        // 对于li+mv，创建li指令
                        new_inst = backend::riscv::RVLi::create(mv_rd, backend::riscv::RVImmediate::create(imm_value));
                        break;
                    case backend::riscv::RVInstType::MV:
                        // 对于mv+mv，直接复制第一个mv的源寄存器到第二个mv的目标寄存器
                        new_inst = backend::riscv::RVMv::create(mv_rd, current_rs1);
                        break;
                    case backend::riscv::RVInstType::ADDIW:
                        // 对于addiw+mv，创建addiw指令
                        new_inst = backend::riscv::RVAddiw::create(
                            mv_rd, current_rs1, backend::riscv::RVImmediate::create(imm_value));
                        break;
                    case backend::riscv::RVInstType::SUB:
                        // 对于sub+mv，创建sub指令
                        if (current_rs2) {
                            new_inst = backend::riscv::RVSub::create(mv_rd, current_rs1, current_rs2);
                        }
                        break;
                    case backend::riscv::RVInstType::XORI:
                        // 对于xori+mv，创建xori指令
                        new_inst = backend::riscv::RVXori::create(
                            mv_rd, current_rs1, backend::riscv::RVImmediate::create(imm_value));
                        break;
                    case backend::riscv::RVInstType::SLLIW:
                        // 对于slliw+mv，创建slliw指令
                        new_inst = backend::riscv::RVSlliw::create(
                            mv_rd, current_rs1, backend::riscv::RVImmediate::create(imm_value));
                        break;
                    case backend::riscv::RVInstType::SRAIW:
                        // 对于sraiw+mv，创建sraiw指令
                        new_inst = backend::riscv::RVSraiw::create(
                            mv_rd, current_rs1, backend::riscv::RVImmediate::create(imm_value));
                        break;
                    default:
                        break;
                }

                if (new_inst) {
                    // 删除当前指令
                    current_instr->delete_self();
                    // 替换MV指令
                    it = next_instr->replace_self(new_inst);
                    is_finished = false;
                    continue;  // 跳过++it，因为删除操作已经更新了迭代器
                }
            }
        }

        ++it;
    }

    return is_finished;
}

bool CalculateOpt::sw_lw_to_sw_mv(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 遍历基本块中的所有指令，检查相邻的sw和lw指令对
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return is_finished;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        // 检查是否有下一条指令
        auto next_it = std::next(it);
        if (next_it == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *it;
        backend::riscv::RVInstruction *next_instr = *next_it;

        // 检查当前指令是否为SW指令
        if (current_instr->is_store_ins()) {
            // 检查下一条指令是否为LW指令
            if (next_instr->is_load_ins()) {
                const auto &sw_operands = current_instr->get_operands();
                const auto &lw_operands = next_instr->get_operands();

                // 获取SW指令的操作数：sw rs2, offset(rs1)
                auto *sw_rs2 = dynamic_cast<backend::riscv::RVReg *>(sw_operands[1]);  // 存储的值
                auto *sw_rs1 = dynamic_cast<backend::riscv::RVReg *>(sw_operands[0]);  // 基址寄存器
                auto *sw_offset = sw_operands[2];                                      // 偏移量

                // 获取LW指令的操作数：lw rd, offset(rs1)
                auto *lw_rd = dynamic_cast<backend::riscv::RVReg *>(lw_operands[0]);   // 目标寄存器
                auto *lw_rs1 = dynamic_cast<backend::riscv::RVReg *>(lw_operands[1]);  // 基址寄存器
                auto *lw_offset = lw_operands[2];                                      // 偏移量

                // 检查基址寄存器是否相同
                if (sw_rs1 && lw_rs1 && sw_rs1 == lw_rs1) {
                    // 检查偏移量是否相同
                    bool offset_match = false;
                    if (sw_offset->get_kind() == backend::riscv::RVOperand::Kind::IMM &&
                        lw_offset->get_kind() == backend::riscv::RVOperand::Kind::IMM) {
                        // 立即数偏移量比较
                        auto *sw_imm = dynamic_cast<backend::riscv::RVImmediate *>(sw_offset);
                        auto *lw_imm = dynamic_cast<backend::riscv::RVImmediate *>(lw_offset);
                        offset_match = (sw_imm && lw_imm && sw_imm->value() == lw_imm->value());
                    } else {
                        // 其他类型的偏移量，比较字符串表示
                        offset_match = (sw_offset->to_string() == lw_offset->to_string());
                    }

                    if (offset_match && sw_rs2 && lw_rd) {
                        // 地址完全匹配，可以优化
                        // 创建mv指令：mv lw_rd, sw_rs2
                        auto *mv_inst = backend::riscv::RVMv::create(lw_rd, sw_rs2);

                        // 替换LW指令为MV指令
                        it = next_instr->replace_self(mv_inst);
                        is_finished = false;
                        continue;
                    }
                }
            }
        }

        ++it;
    }

    return is_finished;
}

bool CalculateOpt::remove_redundant_sw(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 删除多余的SW语句：对同一地址的多次写入只保留最后一次
    auto &instructions = block->get_insts();

    // 如果指令列表为空，直接返回
    if (instructions.empty()) {
        return is_finished;
    }

    // 用于存储每个内存地址的最后一次写入指令
    std::unordered_map<std::string, std::vector<backend::riscv::RVInstruction *>> address_to_stores;

    // 第一遍：收集所有SW指令按地址分组
    for (auto *inst : instructions) {
        if (inst->is_store_ins()) {
            const auto &operands = inst->get_operands();
            auto *base_reg = dynamic_cast<backend::riscv::RVReg *>(operands[0]);  // 基址寄存器
            auto *offset = operands[2];                                           // 偏移量

            if (base_reg && offset) {
                // 构建地址标识符：基址寄存器名 + 偏移量
                std::string address_key = base_reg->to_string() + "+" + offset->to_string();
                address_to_stores[address_key].push_back(inst);
            }
        }
    }

    // 第二遍：对每个地址，检查是否存在Load操作阻止优化
    for (auto &[address_key, store_list] : address_to_stores) {
        if (store_list.size() <= 1) {
            continue;  // 只有一次写入，无需优化
        }

        // 解析地址键
        size_t plus_pos = address_key.find('+');
        if (plus_pos == std::string::npos) continue;

        std::string base_name = address_key.substr(0, plus_pos);
        std::string offset_str = address_key.substr(plus_pos + 1);

        // 从第一个SW到最后一个SW之间，检查是否有对该地址的Load
        auto first_store_it = std::find(instructions.begin(), instructions.end(), store_list.front());
        auto last_store_it = std::find(instructions.begin(), instructions.end(), store_list.back());

        bool has_interfering_load = false;

        // 在范围内查找Load指令
        for (auto it = first_store_it; it != last_store_it && it != instructions.end(); ++it) {
            backend::riscv::RVInstruction *inst = *it;

            // 检查是否为Load指令
            if (inst->is_load_ins()) {
                const auto &load_operands = inst->get_operands();
                auto *load_base = dynamic_cast<backend::riscv::RVReg *>(load_operands[1]);  // Load的基址寄存器
                auto *load_offset = load_operands[2];                                       // Load的偏移量

                if (load_base && load_offset) {
                    // 检查Load地址是否与SW地址相同
                    if (load_base->to_string() == base_name && load_offset->to_string() == offset_str) {
                        has_interfering_load = true;
                        break;
                    }
                }
            } else if (inst->get_type() == backend::riscv::RVInstType::CALL) {
                has_interfering_load = true;
                break;
            }
        }

        // 如果没有干扰的Load，删除除最后一个SW外的所有SW
        if (!has_interfering_load) {
            for (size_t i = 0; i < store_list.size() - 1; ++i) {
                // logger::INFO("删除多余的SW指令: ", store_list[i]->to_string());
                store_list[i]->delete_self();
                is_finished = false;
            }
        }
    }

    return is_finished;
}

bool CalculateOpt::mv_instruction_optimization(backend::riscv::RVBasicBlock *block) {
    // 替换并更改位置！
    bool is_finished = true;
    // 优化MV指令，包括向前和向后的寄存器替换优化
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return is_finished;
    }

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *current_instr = *it;
        bool skip_increment = false;

        // 检查当前指令是否为MV指令
        if (current_instr->is_move_ins()) {
            const auto &mv_operands = current_instr->get_operands();
            auto *mv_rd = dynamic_cast<backend::riscv::RVVirReg *>(mv_operands[0]);   // mv的目标寄存器
            auto *mv_rs1 = dynamic_cast<backend::riscv::RVVirReg *>(mv_operands[1]);  // mv的源寄存器

            if (mv_rd && mv_rs1) {
                bool can_optimize = false;

                // 情况1：检查mv的目标寄存器是否只涉及两条指令（定义和使用）
                const auto &mv_rd_uses = mv_rd->get_graph_uses();
                if (mv_rd_uses.size() == 2) {  // 2表示定义和使用
                    // 向下遍历检查MV指令后面的所有指令是否使用了目标寄存器
                    backend::riscv::RVInstruction *instr_to_replace = nullptr;

                    for (auto next_it = std::next(it); next_it != instructions.end(); ++next_it) {
                        backend::riscv::RVInstruction *next_instr = *next_it;
                        const auto &next_uses = next_instr->get_uses();

                        for (auto *operand : next_uses) {
                            if (operand == mv_rd) {
                                instr_to_replace = next_instr;
                                can_optimize = true;
                                break;
                            }
                        }

                        if (can_optimize) {
                            break;
                        }
                    }

                    if (can_optimize) {
                        instr_to_replace->replace_uses(mv_rd, mv_rs1);

                        // 删除mv指令
                        it = current_instr->delete_self();
                        is_finished = false;
                        continue;  // 跳过++it，因为删除操作已经更新了迭代器
                    }
                }

                // 情况2：如果情况1不满足，检查mv的源寄存器是否只涉及两条指令
                const auto &mv_rs1_uses = mv_rs1->get_graph_uses();
                if (mv_rs1_uses.size() == 2) {  // 2表示定义和使用
                    // logger::INFO(current_instr->to_string());
                    // 向上遍历查找写入源寄存器的指令
                    for (auto prev_it = instructions.begin(); prev_it != it; ++prev_it) {
                        backend::riscv::RVInstruction *prev_instr = *prev_it;
                        auto *prev_def = prev_instr->get_def();

                        if (prev_def == mv_rs1) {
                            // 检查待替换指令和MV指令之间是否有其他指令使用了mv_rd
                            bool can_replace = true;
                            for (auto check_it = std::next(prev_it); check_it != it; ++check_it) {
                                backend::riscv::RVInstruction *check_instr = *check_it;
                                const auto &check_uses = check_instr->get_uses();

                                for (auto *operand : check_uses) {
                                    if (operand == mv_rd) {
                                        can_replace = false;
                                        break;
                                    }
                                }

                                if (!can_replace) {
                                    break;
                                }
                            }

                            if (can_replace) {
                                // 找到写入源寄存器的指令，将其改为写入目标寄存器
                                prev_instr->replace_def(mv_rd);

                                // 删除mv指令
                                it = current_instr->delete_self();
                                is_finished = false;
                                skip_increment = true;  // 标记跳过迭代器增加
                                break;                  // 跳出内层循环，继续外层循环
                            }
                        }
                    }
                }
            }
        }

        if (!skip_increment) {
            ++it;
        }
    }

    return is_finished;
}

bool CalculateOpt::pre_ra_addi_sp_reuse(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // logger::INFO("pre_ra_addi_sp_reuse");
    const int range = 16;
    std::unordered_map<int, std::pair<backend::riscv::RVReg *, int>> map;
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end();) {
        backend::riscv::RVInstruction *instr = *it;
        if (dynamic_cast<riscv::RVPhyReg *>(instr->get_def())) {
            ++it;
            continue;
        }

        // 检查指令是否有定义操作数，如果有且不是ADDI指令，则删除所有重定义的寄存器
        if (instr->get_type() != backend::riscv::RVInstType::ADDI) {
            if (instr->get_type() == backend::riscv::RVInstType::CALL) {
                for (auto map_it = map.begin(); map_it != map.end();) {
                    if (riscv::reg_info::is_callee_saved_reg(map_it->second.first)) {
                        map_it = map.erase(map_it);
                    } else {
                        ++map_it;
                    }
                }
            } else {
                auto *def_reg = instr->get_def();
                if (def_reg) {
                    // logger::INFO("remove: ", instr->to_string());
                    for (auto map_it = map.begin(); map_it != map.end();) {
                        if (map_it->second.first == def_reg) {
                            // logger::INFO("ok");
                            map_it = map.erase(map_it);
                        } else {
                            ++map_it;
                        }
                    }
                }
            }
        }

        // 检查是否为ADDI指令且操作数为sp寄存器
        if (instr->get_type() == backend::riscv::RVInstType::ADDI) {
            // logger::INFO(instr->to_string());
            // 获取ADDI指令的操作数：rd, rs1, imm
            auto &operands = instr->get_operands();
            auto *rd_reg = dynamic_cast<backend::riscv::RVReg *>(operands[0]);
            auto *rs1_reg = dynamic_cast<backend::riscv::RVReg *>(operands[1]);
            auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(operands[2]);

            // 检查是否为 addi rd, sp, offset 的形式
            if (rs1_reg == riscv::reg_info::get_sp() && imm) {
                int offset = imm->value();

                // 检查是否已有相同偏移的映射
                auto map_it = map.find(offset);
                if (map_it != map.end()) {
                    // 如果前面有记录这个偏移值，那么就将这个addi给删掉，然后将后面的所有使用这个的寄存器都换掉
                    auto *now_reg = map_it->second.first;
                    if (now_reg != rd_reg) {
                        // 先复制使用列表，避免迭代器失效
                        auto uses = rd_reg->get_graph_uses();
                        for (auto *use_instr : uses) {
                            use_instr->replace_regs(rd_reg, now_reg);
                        }
                    }
                    map_it->second.second = range;  // 刷新生存周期

                    // 删除重复的ADDI指令
                    // logger::INFO("deleted instr: ", instr->to_string());
                    it = (*it)->delete_self();
                    is_finished = false;
                    continue;  // 跳过++it，因为erase已经更新了迭代器
                } else {
                    // 删除所有使用rd_reg的映射
                    for (auto map_it2 = map.begin(); map_it2 != map.end();) {
                        if (map_it2->second.first == rd_reg) {
                            map_it2 = map.erase(map_it2);
                        } else {
                            ++map_it2;
                        }
                    }
                    // 添加新的映射
                    map[offset] = std::make_pair(rd_reg, range);
                }
            }
        }

        // 更新生存周期计数并删除生存周期为0的映射
        for (auto map_it = map.begin(); map_it != map.end();) {
            map_it->second.second--;
            if (map_it->second.second == 0) {
                map_it = map.erase(map_it);
            } else {
                ++map_it;
            }
        }

        ++it;
    }
    return is_finished;
}

bool CalculateOpt::li_zero_optimization(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 记录所有加载0的指令的def_reg和对应的指令
    std::unordered_map<backend::riscv::RVReg *, backend::riscv::RVInstruction *> li_zero_instrs;
    auto &instructions = block->get_insts();

    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
        backend::riscv::RVInstruction *instr = *it;

        // 检查是否为LI指令且加载值为0
        if (instr->get_type() == backend::riscv::RVInstType::LI) {
            const auto &operands = instr->get_operands();
            auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(operands[1]);

            if (imm && imm->value() == 0) {
                auto *def_reg = instr->get_def();
                // 记录这个def_reg和对应的指令
                li_zero_instrs[def_reg] = instr;
            }
        } else if (instr->get_type() == backend::riscv::RVInstType::CALL) {
            for (auto map_it = li_zero_instrs.begin(); map_it != li_zero_instrs.end();) {
                if (riscv::reg_info::is_callee_saved_reg(map_it->first)) {
                    map_it = li_zero_instrs.erase(map_it);
                } else {
                    ++map_it;
                }
            }
        } else {
            if (!instr->is_move_ins()) {
                // 防止出现 mv %int0, zero
                // 检查该指令是否使用了已记录的def_reg
                const auto &uses = instr->get_uses();
                for (auto *use_reg : uses) {
                    if (li_zero_instrs.find(use_reg) != li_zero_instrs.end()) {
                        // 使用则替换为zero
                        instr->replace_uses(use_reg, backend::riscv::reg_info::get_zero());
                        is_finished = false;
                    }
                }
            }

            // 检查该指令写入的寄存器是否存在于已记录的def_reg中
            auto *def_reg = instr->get_def();
            if (def_reg && li_zero_instrs.find(def_reg) != li_zero_instrs.end()) {
                // 有则删去
                li_zero_instrs.erase(def_reg);
            }
        }
    }

    for (auto [reg, instr] : li_zero_instrs) {
        // li a0, 0
        if (dynamic_cast<riscv::RVPhyReg *>(reg)) continue;
        // 检查该寄存器是否只有一个使用（只有定义）
        const auto &uses = reg->get_graph_uses();
        if (uses.size() == 1) {  // 只有定义，没有其他使用
            // 直接删除这个LI指令
            instr->delete_self();
            is_finished = false;
        }
    }

    return is_finished;
}

bool CalculateOpt::optimize_single_block_loop(backend::riscv::RVFunction *function,
                                              std::list<backend::riscv::RVBasicBlock *>::iterator &it) {
    bool is_finished = true;

    auto *block = *it;

    // 检查基本块是否只有两条指令
    auto &instructions = block->get_insts();
    if (instructions.size() != 2) {
        return is_finished;
    }

    // 检查第一条指令是否为分支指令，第二条是否为跳转指令
    auto first_inst = instructions.front();
    auto second_inst = instructions.back();

    if (!first_inst->is_branch_ins() || second_inst->get_type() != backend::riscv::RVInstType::J) {
        return is_finished;
    }

    // 获取分支指令的目标块
    const auto &branch_operands = first_inst->get_operands();
    auto *target_label = dynamic_cast<backend::riscv::RVLabel *>(branch_operands[2]);

    // 查找目标块
    backend::riscv::RVBasicBlock *target_block = nullptr;
    for (auto *func_block : function->get_blocks()) {
        if (func_block->get_name() == target_label->name()) {
            target_block = func_block;
            break;
        }
    }

    if (!target_block) {
        return is_finished;
    }

    auto &target_instructions = target_block->get_insts();

    // 从branch_operands中识别loop_var
    backend::riscv::RVReg *loop_var = nullptr;
    int increment_value = 0;
    int shift_amount = 0;

    // 检查branch_operands中的寄存器，找到loop_var
    for (size_t i = 0; i < 2; ++i) {
        auto *branch_reg = dynamic_cast<backend::riscv::RVReg *>(branch_operands[i]);
        // 在target_block中查找这个寄存器是否被addiw指令自增
        for (auto it = target_instructions.begin(); it != target_instructions.end(); ++it) {
            auto *inst = *it;
            if (inst->get_type() == backend::riscv::RVInstType::ADDIW) {
                const auto &addiw_operands = inst->get_operands();
                auto *rd = dynamic_cast<backend::riscv::RVReg *>(addiw_operands[0]);
                auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(addiw_operands[1]);
                auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(addiw_operands[2]);

                // 检查是否为自增指令且使用了branch_operands中的寄存器
                if (rd == rs1 && rd == branch_reg) {
                    // 检查target_block中是否有j指令指向block，从addiw指令之后开始查找
                    bool found_j_to_block = false;
                    auto j_search_it = std::next(it);
                    for (; j_search_it != target_instructions.end(); ++j_search_it) {
                        auto *check_inst = *j_search_it;
                        if (check_inst->get_type() == backend::riscv::RVInstType::J) {
                            const auto &j_operands = check_inst->get_operands();
                            auto *j_target_label = dynamic_cast<backend::riscv::RVLabel *>(j_operands[0]);
                            if (j_target_label && j_target_label->name() == block->get_name()) {
                                found_j_to_block = true;
                                break;
                            }
                        }
                    }

                    if (found_j_to_block) {
                        loop_var = rd;
                        increment_value = imm->value();
                        break;
                    }
                }
            }
        }
        if (loop_var) {
            break;  // 找到了loop_var，退出外层循环
        }
    }

    if (!loop_var) {
        return is_finished;
    }

    // 检查所有slliw指令的偏移量是否相同，同时获取基地址
    bool found_slliw = false;
    int first_shift_amount = 0;
    std::vector<backend::riscv::RVReg *> base_addr_regs;
    std::vector<backend::riscv::RVReg *> slliw_rd_regs;

    for (auto it = target_instructions.begin(); it != target_instructions.end(); ++it) {
        auto *inst = *it;
        if (inst->get_type() == backend::riscv::RVInstType::SLLIW) {
            const auto &slliw_operands = inst->get_operands();
            auto *rd = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[0]);
            auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[1]);
            auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(slliw_operands[2]);

            if (rd && rs1 && imm && rs1 == loop_var) {
                // 检查下一条指令是否为add指令
                auto next_it = std::next(it);
                if (next_it != target_instructions.end()) {
                    auto *next_inst = *next_it;
                    if (next_inst->get_type() == backend::riscv::RVInstType::ADD) {
                        const auto &add_operands = next_inst->get_operands();
                        auto *add_rd = dynamic_cast<backend::riscv::RVReg *>(add_operands[0]);
                        auto *add_rs1 = dynamic_cast<backend::riscv::RVReg *>(add_operands[1]);
                        auto *add_rs2 = dynamic_cast<backend::riscv::RVReg *>(add_operands[2]);

                        if (!found_slliw) {
                            // 第一个slliw指令，记录偏移量和基地址
                            first_shift_amount = imm->value();
                            if (add_rs1 == rd) {
                                base_addr_regs.push_back(add_rs2);
                            } else if (add_rs2 == rd) {
                                base_addr_regs.push_back(add_rs1);
                            }
                            slliw_rd_regs.push_back(rd);
                            found_slliw = true;
                        } else {
                            // 后续的slliw指令，检查偏移量是否相同
                            if (imm->value() != first_shift_amount) {
                                return is_finished;  // 偏移量不同，退出
                            }
                            // 记录这组的基地址和rd寄存器
                            if (add_rs1 == rd) {
                                base_addr_regs.push_back(add_rs2);
                            } else if (add_rs2 == rd) {
                                base_addr_regs.push_back(add_rs1);
                            }
                            slliw_rd_regs.push_back(rd);
                        }
                    } else {
                        // 下一条指令不是add，不能优化
                        return is_finished;
                    }
                } else {
                    // 没有下一条指令，不能优化
                    return is_finished;
                }
            }
        }
    }

    if (!found_slliw || first_shift_amount == 0 || base_addr_regs.empty()) {
        return is_finished;
    }

    shift_amount = first_shift_amount;

    // 匹配成功，清除自身操作数占用
    auto b_type = first_inst->get_type();
    auto j_label = second_inst->get_operands()[0];
    first_inst->delete_graph_uses();
    second_inst->delete_graph_uses();

    // 加载偏移量
    int offset = increment_value << shift_amount;
    auto *new_imm = backend::riscv::RVImmediate::create(offset);
    backend::riscv::RVVirReg *imm_reg = nullptr;
    backend::riscv::RVInstruction *li_imm = nullptr;
    if (offset >= 2048 || offset < -2048) {
        imm_reg = backend::riscv::RVVirReg::create(
            function->get_next_vreg_index(), backend::riscv::RVVirReg::RegType::INT_TYPE, function);
        li_imm = backend::riscv::RVLi::create(imm_reg, new_imm);
    }

    // 创建新的基本块A和B
    std::string block_a_name = block->get_name() + "_opt_a";
    std::string block_b_name = block->get_name() + "_opt_b";

    auto *block_a = function->new_block(block_a_name);
    auto *block_b = function->new_block(block_b_name);
    block_a->set_loop_depth(block->get_loop_depth() - 1);
    block_b->set_loop_depth(block->get_loop_depth());
    auto *new_label_a = backend::riscv::RVLabel::create(block_a_name);
    auto *new_label_b = backend::riscv::RVLabel::create(block_b_name);

    // 为每组slliw-add组合创建寄存器
    std::vector<backend::riscv::RVReg *> group_regs;
    for (size_t i = 0; i < base_addr_regs.size(); ++i) {
        auto *group_reg = backend::riscv::RVVirReg::create(
            function->get_next_vreg_index(), backend::riscv::RVVirReg::RegType::INT_TYPE, function);
        group_regs.push_back(group_reg);
    }

    // 1是loop_var的，2是比较量的
    auto *new_reg1 = backend::riscv::RVVirReg::create(
        function->get_next_vreg_index(), backend::riscv::RVVirReg::RegType::INT_TYPE, function);
    auto *new_reg2 = backend::riscv::RVVirReg::create(
        function->get_next_vreg_index(), backend::riscv::RVVirReg::RegType::INT_TYPE, function);

    // 创建基本块A的指令
    auto *slliw1 =
        backend::riscv::RVSlliw::create(new_reg1, loop_var, backend::riscv::RVImmediate::create(shift_amount));

    // 找到branch_operands中的比较量
    backend::riscv::RVReg *other_reg = nullptr;
    if (dynamic_cast<backend::riscv::RVReg *>(branch_operands[0]) == loop_var) {
        other_reg = dynamic_cast<backend::riscv::RVReg *>(branch_operands[1]);
    } else {
        other_reg = dynamic_cast<backend::riscv::RVReg *>(branch_operands[0]);
    }

    if (!other_reg) {
        assert(false);
    }

    auto *slliw2 =
        backend::riscv::RVSlliw::create(new_reg2, other_reg, backend::riscv::RVImmediate::create(shift_amount));

    // 创建add指令，将new_reg1和new_reg2都加上基地址
    auto *add2 = backend::riscv::RVAdd::create(new_reg2, new_reg2, base_addr_regs[0]);
    auto *j_to_b = backend::riscv::RVJ::create(new_label_b);

    block_a->add_inst(slliw1);

    // 为每组slliw-add组合添加指令，跳过第一个
    for (size_t i = 0; i < group_regs.size(); ++i) {
        // 创建add指令，加上对应的基地址
        auto *group_add = backend::riscv::RVAdd::create(group_regs[i], new_reg1, base_addr_regs[i]);

        block_a->add_inst(group_add);
    }

    block_a->add_inst(slliw2);
    block_a->add_inst(add2);

    block_a->add_inst(j_to_b);

    // 创建基本块B的指令
    backend::riscv::RVInstruction *branch_inst = nullptr;

    // 确定两个操作数
    backend::riscv::RVReg *first_operand = nullptr;
    backend::riscv::RVReg *second_operand = nullptr;

    if (dynamic_cast<backend::riscv::RVReg *>(branch_operands[0]) == loop_var) {
        first_operand = group_regs[0];
        second_operand = new_reg2;
    } else {
        first_operand = new_reg2;
        second_operand = group_regs[0];
    }

    switch (b_type) {
        case backend::riscv::RVInstType::BLT:
            branch_inst = backend::riscv::RVBlt::create(first_operand, second_operand, target_label);
            break;
        case backend::riscv::RVInstType::BGE:
            branch_inst = backend::riscv::RVBge::create(first_operand, second_operand, target_label);
            break;
        case backend::riscv::RVInstType::BEQ:
            branch_inst = backend::riscv::RVBeq::create(first_operand, second_operand, target_label);
            break;
        case backend::riscv::RVInstType::BNE:
            branch_inst = backend::riscv::RVBne::create(first_operand, second_operand, target_label);
            break;
        case backend::riscv::RVInstType::BLE:
            branch_inst = backend::riscv::RVBle::create(first_operand, second_operand, target_label);
            break;
        case backend::riscv::RVInstType::BGT:
            branch_inst = backend::riscv::RVBgt::create(first_operand, second_operand, target_label);
            break;
        default:
            assert(false);  // 不支持的分支类型
    }
    auto *j_inst = backend::riscv::RVJ::create(dynamic_cast<backend::riscv::RVLabel *>(j_label));

    block_b->add_inst(branch_inst);
    block_b->add_inst(j_inst);

    // 修改所有指向当前块的分支指令和J指令的目标标签为指向块A或块B
    for (auto *func_block : function->get_blocks()) {
        auto &func_instructions = func_block->get_insts();
        for (auto *inst : func_instructions) {
            // 检查是否为分支指令或J指令
            if (inst->is_branch_ins() || inst->get_type() == backend::riscv::RVInstType::J) {
                const auto &operands = inst->get_operands();
                // 分支指令和J指令的目标标签通常是最后一个操作数
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands.back());
                if (label && label->name() == block->get_name()) {
                    // 如果当前遍历的block是target_block，则替换成block_b对应的label
                    // 否则替换成block_a对应的label
                    if (func_block == target_block) {
                        inst->replace_target_label(new_label_b);
                    } else {
                        if (li_imm) {
                            inst->insert_inst_before_self(li_imm->clone());
                        }
                        inst->replace_target_label(new_label_a);
                    }
                }
            }
        }
    }

    // 修改目标块中的addiw指令
    for (auto *inst : target_instructions) {
        if (inst->get_type() == backend::riscv::RVInstType::ADDIW) {
            const auto &addiw_operands = inst->get_operands();
            auto *rd = dynamic_cast<backend::riscv::RVReg *>(addiw_operands[0]);
            auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(addiw_operands[1]);

            if (rd == rs1 && rs1 == loop_var) {
                // 检查指令中j指令前的指令，如果有使用了loop_var而且不是slliw的指令，就insert_inst_after_self
                // 否则说明可以放心删除，replace_self
                bool should_insert_before = false;
                for (auto *check_inst : target_instructions) {
                    if (check_inst->get_type() == backend::riscv::RVInstType::J) {
                        // 找到j指令，停止检查
                        break;
                    }
                    // 检查指令是否使用了loop_var且不是slliw
                    if (check_inst != inst && check_inst->get_type() != backend::riscv::RVInstType::SLLIW) {
                        const auto &regs = check_inst->get_uses();
                        for (auto *reg : regs) {
                            if (reg == loop_var) {
                                should_insert_before = true;
                                break;
                            }
                        }
                        if (should_insert_before) break;
                    }
                }

                // 为每组slliw-add组合创建对应的addi指令
                std::vector<backend::riscv::RVInstruction *> addi_instructions;

                for (auto &group_reg : group_regs) {
                    if (imm_reg) {
                        auto *new_add = backend::riscv::RVAdd::create(group_reg, group_reg, imm_reg);
                        addi_instructions.push_back(new_add);
                    } else {
                        auto *new_addi = backend::riscv::RVAddi::create(group_reg, group_reg, new_imm);
                        addi_instructions.push_back(new_addi);
                    }
                }

                for (auto *addi_inst : addi_instructions) {
                    inst->insert_inst_before_self(addi_inst);
                }

                // 检查loop_var是否在其他基本块中被使用
                bool can_delete = !should_insert_before;
                if (can_delete) {
                    // 检查loop_var的get_graph_uses中的指令，如果指令的parent_block不是target_block，说明在其他block有使用
                    const auto &graph_uses = loop_var->get_graph_uses();
                    for (auto *use_inst : graph_uses) {
                        // 还要检查指令的get_uses是否有loop_var，有才说明是使用不是写入
                        const auto &uses = use_inst->get_uses();
                        if (std::find(uses.begin(), uses.end(), loop_var) != uses.end() &&
                            use_inst->get_parent_block() != target_block && use_inst->get_parent_block() != block_a) {
                            can_delete = false;
                            break;
                        }
                    }
                }

                if (can_delete) {
                    inst->delete_self();
                }
                break;
            }
        }
    }

    // 修改slliw和add指令组合
    for (auto it = target_instructions.begin(); it != target_instructions.end();) {
        auto *inst = *it;
        if (inst->get_type() == backend::riscv::RVInstType::SLLIW) {
            const auto &slliw_operands = inst->get_operands();
            auto *rd = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[0]);
            auto *rs1 = dynamic_cast<backend::riscv::RVReg *>(slliw_operands[1]);
            auto *imm = dynamic_cast<backend::riscv::RVImmediate *>(slliw_operands[2]);

            if (rs1 == loop_var && imm->value() == shift_amount) {
                // 检查下一条指令是否为add
                auto next_it = std::next(it);
                if (next_it != target_instructions.end()) {
                    auto *next_inst = *next_it;
                    if (next_inst->get_type() == backend::riscv::RVInstType::ADD) {
                        const auto &add_operands = next_inst->get_operands();
                        auto *add_rd = dynamic_cast<backend::riscv::RVReg *>(add_operands[0]);
                        auto *add_rs1 = dynamic_cast<backend::riscv::RVReg *>(add_operands[1]);
                        auto *add_rs2 = dynamic_cast<backend::riscv::RVReg *>(add_operands[2]);

                        if (add_rs2 == rd || add_rs1 == rd) {
                            // 找到对应的组索引
                            size_t group_index = 0;
                            for (size_t i = 0; i < slliw_rd_regs.size(); ++i) {
                                if (slliw_rd_regs[i] == rd) {
                                    group_index = i;
                                    break;
                                }
                            }

                            // 先复制使用列表，避免迭代器失效
                            auto add_rd_uses = add_rd->get_graph_uses();
                            for (riscv::RVInstruction *add_rd_inst : add_rd_uses) {
                                add_rd_inst->replace_regs(add_rd, group_regs[group_index]);
                            }

                            it = next_inst->delete_self();
                            // 删除slliw指令
                            if (rd->get_graph_uses().size() == 1) {
                                inst->delete_self();
                            }
                            continue;
                        }
                    }
                }
            }
        }
        ++it;
    }

    // 重新遍历target_block的指令，匹配slliw+add组合并优化
    for (auto it = target_instructions.begin(); it != target_instructions.end();) {
        auto *inst = *it;
        if (inst->get_type() == backend::riscv::RVInstType::SLLIW) {
            // 检查下一条指令是否为add
            auto next_it = std::next(it);
            if (next_it != target_instructions.end()) {
                auto *next_inst = *next_it;
                if (next_inst->get_type() == backend::riscv::RVInstType::ADD) {
                    const auto &add_operands = next_inst->get_operands();
                    auto *add_rd = dynamic_cast<backend::riscv::RVReg *>(add_operands[0]);
                    auto *add_rs1 = dynamic_cast<backend::riscv::RVReg *>(add_operands[1]);
                    auto *add_rs2 = dynamic_cast<backend::riscv::RVReg *>(add_operands[2]);

                    // 检查add指令中是否使用了group_reg中的寄存器
                    backend::riscv::RVReg *group_reg_used = nullptr;
                    if (std::find(group_regs.begin(), group_regs.end(), add_rs1) != group_regs.end()) {
                        group_reg_used = add_rs1;
                    } else if (std::find(group_regs.begin(), group_regs.end(), add_rs2) != group_regs.end()) {
                        group_reg_used = add_rs2;
                    }

                    if (group_reg_used) {
                        // 检查slliw指令的rs1寄存器是否为循环不变量
                        if (!is_loop_invariant_slliw_rs1(inst, target_block)) {
                            // 如果不是循环不变量，跳过这个优化
                            ++it;
                            continue;
                        }

                        // 在block_a中找到写入group_reg_used的最后一条指令
                        auto &block_a_instructions = block_a->get_insts();
                        backend::riscv::RVInstruction *insert_inst = nullptr;

                        // 倒序查找写入group_reg_used的最后一条指令
                        for (auto rit = block_a_instructions.rbegin(); rit != block_a_instructions.rend(); ++rit) {
                            auto *check_inst = *rit;
                            if (check_inst->get_def() == group_reg_used) {
                                insert_inst = check_inst;
                                break;
                            }
                        }

                        if (insert_inst) {
                            next_inst->replace_regs(add_rd, group_reg_used);
                            // 使用clone方法创建新的slliw和add指令
                            auto *new_slliw = inst->clone();
                            auto *new_add = next_inst->clone();

                            insert_inst->insert_inst_after_self(new_add);
                            insert_inst->insert_inst_after_self(new_slliw);

                            // 如果group_reg_used有关loop_var，还要在block_a中找到写入other_reg的最后一条指令，加上同样的偏移
                            if (group_reg_used == group_regs[0]) {
                                backend::riscv::RVInstruction *other_insert_inst = nullptr;

                                // 倒序查找写入other_reg的最后一条指令
                                for (auto rit2 = block_a_instructions.rbegin(); rit2 != block_a_instructions.rend();
                                     ++rit2) {
                                    auto *check_inst2 = *rit2;
                                    if (check_inst2->get_def() == new_reg2) {
                                        other_insert_inst = check_inst2;
                                        break;
                                    }
                                }

                                if (other_insert_inst) {
                                    auto *new_other_add =
                                        backend::riscv::RVAdd::create(new_reg2, new_reg2, inst->get_def());

                                    other_insert_inst->insert_inst_after_self(new_other_add);
                                }
                            }

                            // 先复制使用列表，避免迭代器失效
                            auto add_rd_uses = add_rd->get_graph_uses();
                            for (riscv::RVInstruction *add_rd_inst : add_rd_uses) {
                                add_rd_inst->replace_regs(add_rd, group_reg_used);
                            }

                            it = next_inst->delete_self();
                            inst->delete_self();
                            continue;
                        }
                    }
                }
            }
        }
        ++it;
    }

    // 删除当前基本块并更新迭代器
    it = function->remove_block(block);
    function->recalculate_cfg();

    is_finished = false;
    return is_finished;
}

bool CalculateOpt::optimize_li_block_inlining(backend::riscv::RVFunction *function,
                                              std::list<backend::riscv::RVBasicBlock *>::iterator &it) {
    bool is_finished = true;
    auto *block = *it;
    auto &instructions = block->get_insts();

    // 检查基本块是否至少包含3条指令
    if (instructions.size() < 3) {
        return is_finished;
    }

    // 检查最后两条指令是否为分支指令和跳转指令
    auto last_inst = instructions.back();
    auto second_last_inst = std::prev(instructions.end(), 2);
    auto *second_last_inst_ptr = *second_last_inst;

    if (!second_last_inst_ptr->is_branch_ins() || last_inst->get_type() != backend::riscv::RVInstType::J) {
        return is_finished;
    }

    // 检查前面的指令是否都是LI指令
    std::vector<backend::riscv::RVInstruction *> li_instructions;
    auto it_inst = instructions.begin();

    // 跳过最后两条指令（分支和跳转）
    auto end_it = std::prev(instructions.end(), 2);

    for (; it_inst != end_it; ++it_inst) {
        auto *inst = *it_inst;
        if (inst->get_type() != backend::riscv::RVInstType::LI) {
            return is_finished;  // 如果不是LI指令，不满足条件
        }
        li_instructions.push_back(inst);
    }

    if (li_instructions.empty()) {
        return is_finished;  // 没有LI指令
    }

    // 收集所有指向当前块的分支和跳转指令
    std::vector<backend::riscv::RVInstruction *> target_instructions;

    for (auto *func_block : function->get_blocks()) {
        if (func_block == block) {
            continue;  // 跳过当前块
        }

        auto &func_instructions = func_block->get_insts();
        for (auto *inst : func_instructions) {
            // 检查是否为分支指令或跳转指令
            if (inst->is_branch_ins() || inst->get_type() == backend::riscv::RVInstType::J) {
                const auto &operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands.back());

                if (label && label->name() == block->get_name()) {
                    target_instructions.push_back(inst);
                }
            }
        }
    }

    if (target_instructions.empty()) {
        return is_finished;  // 没有指向当前块的分支/跳转指令
    }

    // 在目标指令前插入LI指令的克隆
    for (auto &target_inst : target_instructions) {
        // 为每个LI指令创建克隆并插入到目标指令前
        for (auto *li_inst : li_instructions) {
            // 在目标指令前插入
            target_inst->insert_inst_before_self(li_inst->clone());
        }
    }

    // 删除原块中的所有LI指令
    for (auto *li_inst : li_instructions) {
        li_inst->delete_self();
    }

    is_finished = false;
    return is_finished;
}

bool CalculateOpt::merge_consecutive_slliw(backend::riscv::RVBasicBlock *block) {
    bool is_finished = true;
    // 遍历基本块中的所有指令，检查相邻的SLLIW指令对
    auto &instructions = block->get_insts();

    // 如果指令列表为空或只有一个指令，直接返回
    if (instructions.size() < 2) {
        return is_finished;
    }

    for (auto iter = instructions.begin(); iter != instructions.end();) {
        // 检查是否有下一条指令
        auto next_iter = std::next(iter);
        if (next_iter == instructions.end()) {
            break;
        }

        backend::riscv::RVInstruction *current_instr = *iter;
        backend::riscv::RVInstruction *next_instr = *next_iter;
        backend::riscv::RVInstruction *new_slliw = nullptr;

        // 检查当前指令是否为SLLIW指令
        if (current_instr->get_type() == backend::riscv::RVInstType::SLLIW) {
            // 检查下一条指令是否为SLLIW指令
            if (next_instr->get_type() == backend::riscv::RVInstType::SLLIW) {
                const auto &current_operands = current_instr->get_operands();
                const auto &next_operands = next_instr->get_operands();

                // 获取当前SLLIW指令的操作数
                auto *current_rd = dynamic_cast<backend::riscv::RVReg *>(current_operands[0]);
                auto *current_rs1 = dynamic_cast<backend::riscv::RVReg *>(current_operands[1]);
                auto *current_imm = dynamic_cast<backend::riscv::RVImmediate *>(current_operands[2]);

                // 获取下一条SLLIW指令的操作数
                auto *next_rd = dynamic_cast<backend::riscv::RVReg *>(next_operands[0]);
                auto *next_rs1 = dynamic_cast<backend::riscv::RVReg *>(next_operands[1]);
                auto *next_imm = dynamic_cast<backend::riscv::RVImmediate *>(next_operands[2]);

                // 检查当前SLLIW的目标寄存器是否与下一条SLLIW的源寄存器相同
                if (current_rd == next_rs1) {
                    // 检查当前SLLIW的def_reg是否只有两个使用（定义和使用）
                    const auto &current_rd_uses = current_rd->get_graph_uses();
                    if (current_rd_uses.size() == 2) {
                        // 计算总的移位量
                        int total_shift = static_cast<int>(current_imm->value() + next_imm->value());

                        // 检查移位量是否在有效范围内（0到31）
                        if (total_shift >= 0 && total_shift <= 31) {
                            // 创建新的SLLIW指令，直接对原始寄存器进行总移位
                            new_slliw = backend::riscv::RVSlliw::create(
                                next_rd,                                          // 目标寄存器
                                current_rs1,                                      // 源寄存器（第一个SLLIW的源寄存器）
                                backend::riscv::RVImmediate::create(total_shift)  // 总的移位量
                            );
                        } else {
                            assert(false);
                        }
                    }
                }
            }
        }

        if (new_slliw) {
            // 删除当前指令和下一条指令，替换为新的合并指令
            iter = (*iter)->delete_self();
            iter = (*iter)->replace_self(new_slliw);
            is_finished = false;
        } else {
            ++iter;
        }
    }

    return is_finished;
}

bool CalculateOpt::is_loop_invariant_slliw_rs1(backend::riscv::RVInstruction *slliw_inst,
                                               backend::riscv::RVBasicBlock *target_block) {
    // 获取slliw指令的rs1寄存器
    const auto &operands = slliw_inst->get_operands();

    auto *rs1_reg = dynamic_cast<backend::riscv::RVReg *>(operands[1]);

    // 遍历target_block中在slliw之前的指令，检查rs1是否被定义
    auto &instructions = target_block->get_insts();
    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
        auto *inst = *it;

        // 如果找到slliw指令，停止遍历
        if (inst == slliw_inst) {
            break;
        }

        // 检查当前指令是否定义了rs1寄存器
        if (inst->get_def() == rs1_reg) {
            return false;  // rs1在slliw之前被定义，不是循环不变量
        }
    }

    return true;  // rs1在slliw之前没有被定义，是循环不变量
}

}  // namespace backend::opt
