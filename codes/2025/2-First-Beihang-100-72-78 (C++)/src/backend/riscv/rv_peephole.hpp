#ifndef GEECEECEE_RV_PEEPHOLE_HPP
#define GEECEECEE_RV_PEEPHOLE_HPP

#pragma once
#include <set>

#include "rv_basic_block.hpp"
#include "rv_function.hpp"
#include "rv_instruction.hpp"
#include "rv_module.hpp"
#include "rv_operand.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

class PeepHole {
public:
    explicit PeepHole(RVModule *module);
    ~PeepHole() = default;

    // 主优化函数
    void after_ra_run();

    // 数据流优化函数
    void before_ra_run();

    // 最终窥孔优化
    void j_next_optimize();

private:
    RVModule *riscv_module = nullptr;

    // 当前正在处理的函数
    RVFunction *current_function = nullptr;

    // 活跃信息映射
    std::map<RVBasicBlock *, BlockLiveInfo> live_info_map = std::map<RVBasicBlock *, BlockLiveInfo>();

    // 受影响的寄存器集合
    std::set<RVReg *> influenced_reg = std::set<RVReg *>();

    // 主要的窥孔优化函数
    bool peephole();

    // 各种具体的优化函数
    void useless_ls_remove(backend::riscv::RVBasicBlock *block);
    bool move_same_reg_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions);
    bool addi_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions);
    bool store_load_optimize(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions);
    bool remove_useless_stack_fixer(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions);
    bool merge_addi_sp(RVBasicBlock::InstIterator &inst_iter,
                       RVBasicBlock::InstList &instructions,
                       RVBasicBlock *block);
    bool move_useless_mv(RVBasicBlock::InstIterator &inst_iter, RVBasicBlock::InstList &instructions);
    bool merge_block(RVFunction *function, RVBasicBlock *block);
    bool mv_sd_optimize(backend::riscv::RVBasicBlock *block);

    // 数据流优化相关函数
    void loop_peephole();
    void delete_useless_def();
    void delete_useless_label();
    void branch_optimize();

    // 辅助函数
    bool can_delete_addi(RVBasicBlock *block, RVBasicBlock::InstIterator inst_iter, RVReg *reg);
    std::set<RVPhyReg *> get_def_phi_reg(RVInstruction *inst);
    bool check_between_icmp_and_br(RVBasicBlock::InstIterator cmp_iter,
                                   RVBasicBlock::InstIterator br_iter,
                                   RVOperand *op1,
                                   RVOperand *op2,
                                   RVBasicBlock::InstList &instructions);
};

}  // namespace backend::riscv

#endif  // GEECEECEE_RV_PEEPHOLE_HPP
