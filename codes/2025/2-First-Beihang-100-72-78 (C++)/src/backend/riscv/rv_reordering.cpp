#include "rv_reordering.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

#include "log.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

ReOrdering::ReOrdering(RVModule *module) : riscv_module(module) {}

std::vector<RVPhyReg *> ReOrdering::get_call_defs() {
    std::vector<RVPhyReg *> ret;

    std::vector<int> int_regs = {1, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};
    std::vector<int> float_regs = {0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 28, 29, 30, 31};

    // 添加浮点寄存器
    for (int i : float_regs) {
        ret.push_back(reg_info::get_fpu_reg(i));
    }

    // 添加整数寄存器
    for (int i : int_regs) {
        ret.push_back(reg_info::get_cpu_reg(i));
    }

    return ret;
}

int ReOrdering::update_live_out_for_inst(RVInstruction *inst, std::set<RVReg *> &live_out) {
    // 处理MV指令的特殊情况
    if (inst->is_move_ins()) {
        auto uses = inst->get_uses();
        if (!uses.empty() && uses[0]) {
            live_out.erase(uses[0]);
        }
    }

    int cnt = 0;
    std::vector<RVReg *> def_regs;

    // 获取定义寄存器
    RVReg *def_reg = inst->get_def();
    if (def_reg) {
        def_regs.push_back(def_reg);

        // 如果是CALL指令，添加函数调用时被修改的寄存器
        if (inst->get_type() == RVInstType::CALL) {
            auto call_defs = get_call_defs();
            def_regs.insert(def_regs.end(), call_defs.begin(), call_defs.end());
        }

        // 添加到活跃输出集
        for (auto *reg : def_regs) {
            live_out.insert(reg);
        }

        // 计算冲突边数量
        cnt += def_regs.size() * live_out.size();

        // 根据指令类型决定是否从活跃输出集中移除定义寄存器
        if (inst->get_type() != RVInstType::MV && inst->get_type() != RVInstType::FMV_S) {
            for (auto *reg : def_regs) {
                live_out.erase(reg);
            }
        } else if (!dynamic_cast<RVPhyReg *>(def_reg)) {
            // 如果是MV指令且定义寄存器不是物理寄存器，则移除
            for (auto *reg : def_regs) {
                live_out.erase(reg);
            }
        }
    }

    // 添加使用寄存器到活跃输出集
    auto uses = inst->get_uses();
    for (auto *reg : uses) {
        live_out.insert(reg);
    }

    return cnt;
}

bool ReOrdering::has_fix_phy_reg(RVInstruction *inst) {
    // 检查操作数是否为固定的物理寄存器
    auto operands = inst->get_operands();
    for (auto *operand : operands) {
        if (operand) {
            auto *phy_reg = dynamic_cast<RVPhyReg *>(operand);
            if (phy_reg) {
                int phys_id = phy_reg->get_phys_id();
                if (phy_reg->get_type() == RVPhyReg::Type::INT) {
                    // CPU寄存器：ra(1), sp(2), a0-a7(10-17)不能重排序
                    if (phys_id == 1 || phys_id == 2 || (phys_id >= 10 && phys_id <= 17)) {
                        return true;
                    }
                } else {
                    // FPU寄存器：fa0-fa6(10-16)不能重排序
                    if (phys_id >= 10 && phys_id <= 16) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

int ReOrdering::need_reorder(const std::set<RVReg *> &live_out,
                             RVBasicBlock::InstIterator inst_iter,
                             const RVBasicBlock::InstList &instructions) {
    RVInstruction *inst = *inst_iter;

    // 如果是跳转指令，不能重排序
    if (inst->is_jump_ins() || inst->is_branch_ins()) {
        return 0;
    }

    // 如果包含固定的物理寄存器，不能重排序
    if (has_fix_phy_reg(inst)) {
        return 0;
    }

    auto live_out_copy1 = std::set<RVReg *>();
    for (auto *reg : live_out) {
        live_out_copy1.insert(reg);
    }
    int conflict1 = update_live_out_for_inst(inst, live_out_copy1);

    std::vector<RVInstruction *> new_order;
    auto cur_iter = inst_iter;
    int max_valid_reorder_len = 0;

    while (true) {
        // 向前移动迭代器
        --cur_iter;

        // 检查是否到达列表开始
        if (cur_iter == instructions.begin()) {
            return max_valid_reorder_len;
        }

        // 检查迭代器是否有效
        if (cur_iter == instructions.end()) {
            return max_valid_reorder_len;
        }

        RVInstruction *cur_inst = *cur_iter;

        // 检查数据依赖
        RVReg *inst_def = inst->get_def();
        auto inst_uses = inst->get_uses();
        RVReg *cur_def = cur_inst->get_def();
        auto cur_uses = cur_inst->get_uses();

        if (std::find(cur_uses.begin(), cur_uses.end(), inst_def) != cur_uses.end() ||
            std::find(inst_uses.begin(), inst_uses.end(), cur_def) != inst_uses.end()) {
            return max_valid_reorder_len;
        }

        // 检查内存访问冲突
        if (inst->is_memory_ins() && cur_inst->is_memory_ins()) {
            return max_valid_reorder_len;
        }

        // 检查跳转指令
        if (cur_inst->is_jump_ins() || cur_inst->is_branch_ins()) {
            return max_valid_reorder_len;
        }

        // 检查固定物理寄存器
        if (has_fix_phy_reg(cur_inst)) {
            return max_valid_reorder_len;
        }

        new_order.push_back(cur_inst);
        conflict1 += update_live_out_for_inst(cur_inst, live_out_copy1);

        // 计算重排序后的冲突
        auto live_out_copy2 = std::set<RVReg *>();
        for (auto *reg : live_out) {
            live_out_copy2.insert(reg);
        }
        int conflict2 = 0;

        for (auto *new_order_inst : new_order) {
            conflict2 += update_live_out_for_inst(new_order_inst, live_out_copy2);
        }
        conflict2 += update_live_out_for_inst(inst, live_out_copy2);

        // 比较冲突数量
        if (conflict1 > conflict2) {
            max_valid_reorder_len = new_order.size();
        } else if (conflict2 > conflict1) {
            return max_valid_reorder_len;
        }
    }
}

bool ReOrdering::reorder_block(RVBasicBlock *bb, const BlockLiveInfo &live_info) {
    auto live_out = std::set<RVReg *>();
    for (auto *reg : live_info.live_out) {
        live_out.insert(reg);
    }
    bool changed = false;

    auto &instructions = bb->get_insts();

    // 检查instructions是否为空
    if (instructions.empty()) {
        return false;
    }

    // 从后往前遍历指令
    auto iter = instructions.end();
    while (iter != instructions.begin()) {
        --iter;
        RVInstruction *inst = *iter;

        // 计算距离
        int need_reorder_len = need_reorder(live_out, iter, instructions);

        if (need_reorder_len > 0) {
            // 执行替换
            auto target_iter = iter;
            for (int j = 0; j < need_reorder_len && target_iter != instructions.begin(); ++j) {
                --target_iter;
            }

            if (target_iter != iter) {
                (*target_iter)->insert_inst_before_self(*iter);
                // 下面这个erase是清除列表，不是真的删除，不能用delete_self()
                iter = instructions.erase(iter);
                changed = true;
            }
        }
        // 更新活跃信息
        update_live_out_for_inst(inst, live_out);
    }

    return changed;
}

void ReOrdering::run() {
    // auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto &[func_name, func] : riscv_module->get_functions()) {
        // 使用现有的活跃信息分析函数
        func->liveness_analysis();

        // 对每个基本块进行重排序
        for (auto *bb : func->get_blocks()) {
            // 跳过指令数量过多的基本块
            if (bb->get_insts().size() > 1000) {
                continue;
            }

            // 使用不动点算法进行重排序
            bool change = true;

            while (change) {
                change = reorder_block(bb, bb->get_live_info());
                // logger::INFO("change: ", change);
            }
        }
    }

    // auto end_time = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Reordering optimization took " << duration.count() << " ms" << std::endl;
}

}  // namespace backend::riscv
