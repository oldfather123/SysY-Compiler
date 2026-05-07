#ifndef GEECEECEE_RV_FUNCTION_HPP
#define GEECEECEE_RV_FUNCTION_HPP

#pragma once
#include <list>
#include <map>
#include <set>
#include <string>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "rv_basic_block.hpp"
#include "rv_instruction.hpp"
#include "rv_operand.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

class RVVirReg;  // Forward declaration

class RVFunction {
public:
    using Ptr = RVFunction *;

    static Ptr create(std::string name);
    std::string to_string() const;

    RVBasicBlock *new_block(const std::string &name);
    std::list<RVBasicBlock *>::iterator remove_block(RVBasicBlock *block);
    void remove_empty_blocks();  // 删除所有空块
    std::string &get_name() { return name; }
    std::list<RVBasicBlock *> &get_blocks() { return blocks; }

    // 判断是否为外部函数（blocks为空）
    bool is_external() const { return blocks.empty(); }

    void alloc_stack(ir::Alloca *alloca);
    int get_alloca_inst_offset(ir::Alloca *alloca);
    bool has_alloca_inst_offset(ir::Alloca *alloca);
    int get_stack_size() const { return stack_size; }
    void set_stack_size(int size) { stack_size = size; }

    // Virtual register management
    void add_virtual_register(RVVirReg *vir_reg);
    const std::set<RVVirReg *> &get_all_vir_reg_used() const { return all_vir_reg_used; }

    int get_next_vreg_index() { return next_vreg_index++; }

    // Stack space management for spilled registers
    void allocate_stack_space(RVVirReg *vir_reg);
    int get_stack_offset(RVVirReg *vir_reg);

    // Virtual register remapping
    void remap_virtual_register(RVVirReg *new_reg, RVVirReg *old_reg);
    void replace_virtual_register(RVVirReg *new_reg, RVVirReg *old_reg);

    // 活跃信息分析
    void liveness_analysis();

    // 重新排序基本块
    void reorder_blocks(const std::vector<int> &new_order, const std::vector<RVBasicBlock *> &original_blocks);

    // 重新计算基本块的前驱和后继关系
    void recalculate_cfg();

    // 新增：参数分配与栈空间映射
    template <typename ArgList, typename ValueToOperandMap>
    void translate_args(const ArgList &args, ValueToOperandMap &value_to_operand, RVBasicBlock *entry_bb) {
        // entry_bb 为空表示解析的是库函数的参数
        int int_arg_idx = 0;
        int float_arg_idx = 0;
        for (auto &arg : args) {
            if (arg->get_type()->is_float_ty()) {
                if (float_arg_idx < 8) {
                    auto *freg = RVVirReg::create(get_next_vreg_index(), RVVirReg::RegType::FLOAT_TYPE, this);
                    auto *arg_reg = reg_info::get_float_arg_reg(float_arg_idx);
                    if (entry_bb) entry_bb->add_inst(RVFmv_s::create(freg, arg_reg));
                    value_to_operand[arg.get()] = freg;
                } else {
                    stack_size += 8;
                    if (!has_arg_offset(arg.get())) {
                        set_arg_offset(arg.get(), stack_size);
                    }
                    // auto* freg = RVVirReg::create(get_next_vreg_index(), RVVirReg::RegType::FLOAT_TYPE, this);
                    // entry_bb->add_inst(RVFlw::create(freg, reg_info::get_sp(), RVImmediate::create(stack_size)));
                    // value_to_operand[arg.get()] = freg;
                }
                float_arg_idx++;
            } else {
                if (int_arg_idx < 8) {
                    auto *reg = RVVirReg::create(get_next_vreg_index(), RVVirReg::RegType::INT_TYPE, this);
                    auto *arg_reg = reg_info::get_arg_reg(int_arg_idx);
                    if (entry_bb) entry_bb->add_inst(RVMv::create(reg, arg_reg));
                    value_to_operand[arg.get()] = reg;
                } else {
                    stack_size += 8;
                    if (!has_arg_offset(arg.get())) {
                        set_arg_offset(arg.get(), stack_size);
                    }
                    // auto* reg = RVVirReg::create(get_next_vreg_index(), RVVirReg::RegType::INT_TYPE, this);
                    // entry_bb->add_inst(RVLw::create(reg, reg_info::get_sp(), RVImmediate::create(stack_size)));
                    // value_to_operand[arg.get()] = reg;
                }
                int_arg_idx++;
            }
        }
        // 先存好，然后在translate_ret中取回来
        if (entry_bb) entry_bb->add_inst(RVMv::create(return_vir_reg, reg_info::get_ra()));
    }

    // 参数偏移相关接口
    bool has_arg_offset(ir::Argument *arg) const { return arg_offset_map.find(arg) != arg_offset_map.end(); }
    void set_arg_offset(ir::Argument *arg, int offset) { arg_offset_map[arg] = offset; }
    int get_arg_offset(ir::Argument *arg) const {
        auto it = arg_offset_map.find(arg);
        if (it == arg_offset_map.end()) return 0;
        return it->second;
    }

    void set_return_vir_reg(RVVirReg *vir_reg) { return_vir_reg = vir_reg; }

    RVVirReg *get_return_vir_reg() { return return_vir_reg; }

    ~RVFunction();  // Destructor

private:
    explicit RVFunction(std::string name);

    std::string name;
    std::list<RVBasicBlock *> blocks = std::list<RVBasicBlock *>();
    int stack_size = 0;
    std::set<RVVirReg *> all_vir_reg_used = std::set<RVVirReg *>();  // 新增
    int next_vreg_index = 0;
    std::map<RVVirReg *, int> vir_reg2stack_offset = std::map<RVVirReg *, int>();  // 虚拟寄存器到栈偏移的映射
    std::map<ir::Argument *, int> arg_offset_map = std::map<ir::Argument *, int>();
    std::map<ir::Alloca *, int> alloca_inst_offset_map = std::map<ir::Alloca *, int>();
    RVVirReg *return_vir_reg = nullptr;
    ;  // 存储函数返回地址

    // 辅助函数：根据标签名称找到对应的基本块
    RVBasicBlock *find_block_by_label(const std::string &label_name);
};

}  // namespace backend::riscv

#endif
