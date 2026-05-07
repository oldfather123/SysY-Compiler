#pragma once

#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"
#include "backend/riscv/rv_operand.hpp"

namespace backend::opt {

class CalculateOpt {
public:
    static void run_before_ra(backend::riscv::RVModule *module);
    static void run_aft_bin(backend::riscv::RVModule *module);

private:
    static void seqz_branch_reverse(backend::riscv::RVBasicBlock *block);
    static bool sra_sll_to_and(backend::riscv::RVBasicBlock *block);
    static void addi_ls_to_ls_offset(backend::riscv::RVBasicBlock *block);
    static bool useless_ls_remove(backend::riscv::RVBasicBlock *block);
    static bool one_to_zero_beq(backend::riscv::RVBasicBlock *block);
    static bool add_zero_to_mv(backend::riscv::RVBasicBlock *block);
    static bool match_eq(backend::riscv::RVInstruction *now,
                         backend::riscv::RVInstruction *next,
                         backend::riscv::RVInstruction *far_next);
    static bool match_ne(backend::riscv::RVInstruction *now,
                         backend::riscv::RVInstruction *next,
                         backend::riscv::RVInstruction *far_next);
    static bool match_sge_and_sle(backend::riscv::RVInstruction *now,
                                  backend::riscv::RVInstruction *next,
                                  backend::riscv::RVInstruction *far_next);
    static bool match_slt_and_sgt(backend::riscv::RVInstruction *now, backend::riscv::RVInstruction *next);
    static bool match_far_next(backend::riscv::RVInstruction *now,
                               backend::riscv::RVInstruction *next,
                               backend::riscv::RVInstruction *far_next);
    static void icmp_branch_to_branch(backend::riscv::RVBasicBlock *block);
    static bool pre_ra_const_value_reuse(backend::riscv::RVBasicBlock *block);
    static bool li_to_lui_or_addiw(backend::riscv::RVBasicBlock *block);
    static bool pre_ra_const_imm_cal_reuse(backend::riscv::RVBasicBlock *block);
    static bool pre_ra_const_pointer_reuse(backend::riscv::RVBasicBlock *block);
    static bool pre_ra_addi_sp_reuse(backend::riscv::RVBasicBlock *block);
    static void aft_bin_const_value_reuse(backend::riscv::RVBasicBlock *block);
    static void remove_same_mv(backend::riscv::RVBasicBlock *block);
    static void sub_zero_remove(backend::riscv::RVBasicBlock *block);
    static bool ls_conflict(
        backend::riscv::RVBasicBlock *block, int i, int j, bool check_in_lw, backend::riscv::RVReg *base, long offset);
    static void lsw_to_lsd(backend::riscv::RVBasicBlock *block);
    static void li_0_to_mv_zero(backend::riscv::RVBasicBlock *block);

    static void mv_copy(backend::riscv::RVReg *bef, backend::riscv::RVReg *aft);
    static backend::riscv::RVReg *li_find(backend::riscv::RVReg *best_reg, long value);
    static void slliw_add_to_shadd(backend::riscv::RVBasicBlock *block);
    static bool li_mv_to_li(backend::riscv::RVBasicBlock *block);
    static bool sw_lw_to_sw_mv(backend::riscv::RVBasicBlock *block);
    static bool remove_redundant_sw(backend::riscv::RVBasicBlock *block);
    static bool mv_instruction_optimization(backend::riscv::RVBasicBlock *block);
    static bool li_zero_optimization(backend::riscv::RVBasicBlock *block);
    static bool optimize_single_block_loop(backend::riscv::RVFunction *function,
                                           std::list<backend::riscv::RVBasicBlock *>::iterator &it);
    static bool optimize_li_block_inlining(backend::riscv::RVFunction *function,
                                           std::list<backend::riscv::RVBasicBlock *>::iterator &it);
    static bool merge_consecutive_slliw(backend::riscv::RVBasicBlock *block);
    static bool is_loop_invariant_slliw_rs1(backend::riscv::RVInstruction *slliw_inst,
                                            backend::riscv::RVBasicBlock *target_block);
};

}  // namespace backend::opt
