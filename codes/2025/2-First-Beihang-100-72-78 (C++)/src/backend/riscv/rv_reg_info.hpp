#ifndef GEECEECEE_RV_REG_INFO_HPP
#define GEECEECEE_RV_REG_INFO_HPP

#pragma once
// #include <memory> // 删除此行
#include <vector>

#include "rv_operand.hpp"

namespace backend::riscv::reg_info {

// Accessors for special purpose registers
RVPhyReg *get_zero();  // x0
RVPhyReg *get_ra();    // x1, return address
RVPhyReg *get_sp();    // x2, stack pointer
RVPhyReg *get_gp();    // x3, global pointer
RVPhyReg *get_tp();    // x4, thread pointer
RVPhyReg *get_a0();

// Accessors for argument registers (a0-a7)
RVPhyReg *get_arg_reg(int index);
RVPhyReg *get_float_arg_reg(int index);

// Accessors for general CPU and FPU registers
RVPhyReg *get_cpu_reg(int index);
RVPhyReg *get_fpu_reg(int index);

// Property queries based on RISC-V ABI
bool is_caller_saved(int phys_id);
bool is_callee_saved(int phys_id);

bool is_float_caller_saved(int phys_id);
bool is_float_callee_saved(int phys_id);

// Get all CPU and FPU registers
std::vector<RVPhyReg *> get_all_cpu_regs();
std::vector<RVPhyReg *> get_all_fpu_regs();

// 新增：释放所有静态分配的寄存器
void free_all_regs();

// 新增：获取函数调用可能更改的物理寄存器集合
std::vector<RVPhyReg *> get_caller_saved_cpu_regs();
std::vector<RVPhyReg *> get_caller_saved_fpu_regs();
std::vector<RVPhyReg *> get_all_caller_saved_regs();

// 新增：判断寄存器是否属于被调用者保存的寄存器
bool is_callee_saved_reg(RVReg *reg);

}  // namespace backend::riscv::reg_info

#endif  // GEECEECEE_RV_REG_INFO_HPP
