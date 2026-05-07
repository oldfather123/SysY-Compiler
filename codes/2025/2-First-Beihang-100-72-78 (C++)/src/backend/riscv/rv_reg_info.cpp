#include "rv_reg_info.hpp"

#include <stdexcept>

namespace backend::riscv::reg_info {

// 静态物理寄存器组
static std::vector<RVPhyReg *> cpu_reg_group = [] {
    std::vector<RVPhyReg *> regs(32);
    for (int i = 0; i < 32; ++i) {
        regs[i] = RVPhyReg::create(i);
    }
    return regs;
}();

static std::vector<RVPhyReg *> fpu_reg_group = [] {
    std::vector<RVPhyReg *> regs(32);
    for (int i = 0; i < 32; ++i) {
        regs[i] = RVPhyReg::create(i, "f" + std::to_string(i), RVPhyReg::Type::FLOAT);
    }
    return regs;
}();

RVPhyReg *get_zero() { return cpu_reg_group[0]; }
RVPhyReg *get_ra() { return cpu_reg_group[1]; }
RVPhyReg *get_sp() { return cpu_reg_group[2]; }
RVPhyReg *get_gp() { return cpu_reg_group[3]; }
RVPhyReg *get_tp() { return cpu_reg_group[4]; }
RVPhyReg *get_a0() { return cpu_reg_group[10]; }

RVPhyReg *get_arg_reg(int index) {
    if (index < 0 || index > 7) {
        throw std::out_of_range("Argument register index must be between 0 and 7");
    }
    // a0-a7 are x10-x17
    return cpu_reg_group[10 + index];
}

RVPhyReg *get_float_arg_reg(int index) {
    if (index < 0 || index > 7) {
        throw std::out_of_range("Float argument register index must be between 0 and 7");
    }
    // fa0-fa7 are f10-f17
    return fpu_reg_group[10 + index];
}

RVPhyReg *get_cpu_reg(int index) {
    if (index < 0 || index > 31) {
        throw std::out_of_range("CPU register index must be between 0 and 31");
    }
    return cpu_reg_group[index];
}

RVPhyReg *get_fpu_reg(int index) {
    if (index < 0 || index > 31) {
        throw std::out_of_range("FPU register index must be between 0 and 31");
    }
    return fpu_reg_group[index];
}

bool is_caller_saved(int phys_id) {
    // ra (x1)
    if (phys_id == 1) return true;
    // t0-t2 (x5-x7)
    if (phys_id >= 5 && phys_id <= 7) return true;
    // a0-a7 (x10-x17)
    if (phys_id >= 10 && phys_id <= 17) return true;
    // t3-t6 (x28-x31)
    if (phys_id >= 28 && phys_id <= 31) return true;
    return false;
}

bool is_callee_saved(int phys_id) {
    // s0-s1 (x8-x9)
    if (phys_id >= 8 && phys_id <= 9) return true;
    // s2-s11 (x18-x27)
    if (phys_id >= 18 && phys_id <= 27) return true;
    return false;
}

bool is_float_caller_saved(int phys_id) {
    // ft0-ft7 (f0-f7)
    if (phys_id >= 0 && phys_id <= 7) return true;
    // fa0-fa7 (f10-f17)
    if (phys_id >= 10 && phys_id <= 17) return true;
    // ft8-ft11 (f28-f31)
    if (phys_id >= 28 && phys_id <= 31) return true;
    return false;
}

bool is_float_callee_saved(int phys_id) {
    // fs0-fs1 (f8-f9)
    if (phys_id >= 8 && phys_id <= 9) return true;
    // fs2-fs11 (f18-f27)
    if (phys_id >= 18 && phys_id <= 27) return true;
    return false;
}

std::vector<RVPhyReg *> get_all_cpu_regs() { return cpu_reg_group; }

std::vector<RVPhyReg *> get_all_fpu_regs() { return fpu_reg_group; }

// 新增：释放所有静态分配的寄存器
void free_all_regs() {
    for (auto *reg : cpu_reg_group) {
        delete reg;
    }
    cpu_reg_group.clear();
    for (auto *reg : fpu_reg_group) {
        delete reg;
    }
    fpu_reg_group.clear();
}

// 新增：获取函数调用可能更改的CPU物理寄存器集合
std::vector<RVPhyReg *> get_caller_saved_cpu_regs() {
    std::vector<RVPhyReg *> caller_saved_regs;

    // ra (x1)
    caller_saved_regs.push_back(cpu_reg_group[1]);

    // t0-t2 (x5-x7)
    for (int i = 5; i <= 7; ++i) {
        caller_saved_regs.push_back(cpu_reg_group[i]);
    }

    // a0-a7 (x10-x17)
    for (int i = 10; i <= 17; ++i) {
        caller_saved_regs.push_back(cpu_reg_group[i]);
    }

    // t3-t6 (x28-x31)
    for (int i = 28; i <= 31; ++i) {
        caller_saved_regs.push_back(cpu_reg_group[i]);
    }

    return caller_saved_regs;
}

// 新增：获取函数调用可能更改的FPU物理寄存器集合
std::vector<RVPhyReg *> get_caller_saved_fpu_regs() {
    std::vector<RVPhyReg *> caller_saved_regs;

    // ft0-ft7 (f0-f7)
    for (int i = 0; i <= 7; ++i) {
        caller_saved_regs.push_back(fpu_reg_group[i]);
    }

    // fa0-fa7 (f10-f17)
    for (int i = 10; i <= 17; ++i) {
        caller_saved_regs.push_back(fpu_reg_group[i]);
    }

    // ft8-ft11 (f28-f31)
    for (int i = 28; i <= 31; ++i) {
        caller_saved_regs.push_back(fpu_reg_group[i]);
    }

    return caller_saved_regs;
}

// 新增：获取所有函数调用可能更改的物理寄存器集合（CPU + FPU）
std::vector<RVPhyReg *> get_all_caller_saved_regs() {
    std::vector<RVPhyReg *> all_caller_saved_regs;

    // 添加CPU caller-saved寄存器
    auto cpu_caller_saved = get_caller_saved_cpu_regs();
    all_caller_saved_regs.insert(all_caller_saved_regs.end(), cpu_caller_saved.begin(), cpu_caller_saved.end());

    // 添加FPU caller-saved寄存器
    auto fpu_caller_saved = get_caller_saved_fpu_regs();
    all_caller_saved_regs.insert(all_caller_saved_regs.end(), fpu_caller_saved.begin(), fpu_caller_saved.end());

    return all_caller_saved_regs;
}

// 新增：判断寄存器是否属于被调用者保存的寄存器
bool is_callee_saved_reg(RVReg *reg) {
    // 检查参数是否为空
    if (!reg) {
        return false;
    }

    // 转换为物理寄存器
    auto *phy_reg = dynamic_cast<RVPhyReg *>(reg);
    if (!phy_reg) {
        return false;
    }

    // 获取物理寄存器ID
    int phys_id = phy_reg->get_phys_id();

    // 根据寄存器类型判断是否为被调用者保存的寄存器
    if (phy_reg->get_type() == RVPhyReg::Type::INT) {
        return is_callee_saved(phys_id);
    } else if (phy_reg->get_type() == RVPhyReg::Type::FLOAT) {
        return is_float_callee_saved(phys_id);
    }

    return false;
}

}  // namespace backend::riscv::reg_info
