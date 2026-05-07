#ifndef GEECEECEE_RV_REORDERING_HPP
#define GEECEECEE_RV_REORDERING_HPP

#pragma once
#include <set>

#include "rv_basic_block.hpp"
#include "rv_module.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

class ReOrdering {
public:
    explicit ReOrdering(RVModule *module);
    ~ReOrdering() = default;

    // 主优化函数
    void run();

private:
    RVModule *riscv_module = nullptr;

    // 获取函数调用时被修改的寄存器列表
    std::vector<RVPhyReg *> get_call_defs();

    // 更新活跃信息并返回冲突边数量
    int update_live_out_for_inst(RVInstruction *inst, std::set<RVReg *> &live_out);

    // 检查指令是否包含固定的物理寄存器
    bool has_fix_phy_reg(RVInstruction *inst);

    int need_reorder(const std::set<RVReg *> &live_out,
                     RVBasicBlock::InstIterator inst_iter,
                     const RVBasicBlock::InstList &instructions);

    // 对基本块进行重排序
    bool reorder_block(RVBasicBlock *bb, const BlockLiveInfo &live_info);
};

}  // namespace backend::riscv

#endif  // GEECEECEE_RV_REORDERING_HPP
