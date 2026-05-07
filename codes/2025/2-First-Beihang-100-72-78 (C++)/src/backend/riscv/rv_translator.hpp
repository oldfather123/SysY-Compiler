#ifndef GEECEECEE_RV_TRANSLATOR_HPP
#define GEECEECEE_RV_TRANSLATOR_HPP

#pragma once
#include <cstdlib>
#include <memory>
#include <vector>

#include "../../ir/mod.hpp"
#include "rv_basic_block.hpp"
#include "rv_function.hpp"
#include "rv_module.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

class RVTranslator {
public:
    static RVModule *translate(ir::Module *ir_module);
    RVVirReg *allocate_register();

private:
    explicit RVTranslator(ir::Module *ir_mod) : ir_module(ir_mod) {}

    RVModule *run();
    void translate_function(std::shared_ptr<ir::Function> ir_func);
    void dfs(std::shared_ptr<ir::BasicBlock> root, std::unordered_set<std::shared_ptr<ir::BasicBlock>> visited);
    void bfs(std::shared_ptr<ir::BasicBlock> root);
    void translate_basic_block(std::shared_ptr<ir::BasicBlock> ir_bb, RVBasicBlock::Ptr rv_bb);
    void translate_instruction(std::shared_ptr<ir::Instruction> ir_inst, RVBasicBlock::Ptr rv_bb);

    // 操作数获取和寄存器分配
    RVOperand *get_operand(RVBasicBlock::Ptr bb, std::shared_ptr<ir::Value> ir_value);
    RVOperand *get_operand_or_imm(RVBasicBlock::Ptr bb, std::shared_ptr<ir::Value> ir_value, const bool _2048_flag);
    RVVirReg *get_res_reg(RVBasicBlock::Ptr bb, std::shared_ptr<ir::Instruction> inst, RVVirReg::RegType reg_type);
    RVVirReg *get_new_vir_reg(RVVirReg::RegType reg_type);
    RVVirReg *allocate_fregister();
    RVVirReg *allocate_virtual_register(RVVirReg::RegType reg_type);
    std::string get_label_name(std::shared_ptr<ir::BasicBlock> bb);

    // 二元操作翻译方法
    void translate_binary_op(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_add(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_sub(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_mul(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_div(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_mod(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_and(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_or(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_xor(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_fbin(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);

    // 其他翻译方法
    void translate_unary_op(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_phi(std::shared_ptr<ir::Phi> phi, RVBasicBlock::Ptr bb);
    void translate_move(std::shared_ptr<ir::Move> move, RVBasicBlock::Ptr bb);
    void translate_load(std::shared_ptr<ir::Load> load, RVBasicBlock::Ptr bb);
    void translate_store(std::shared_ptr<ir::Store> store, RVBasicBlock::Ptr bb);
    void translate_icmp(std::shared_ptr<ir::ICmp> icmp, RVBasicBlock::Ptr bb);
    void translate_fcmp(std::shared_ptr<ir::FCmp> fcmp, RVBasicBlock::Ptr bb);
    void translate_br(const std::shared_ptr<ir::Br> &br, const RVBasicBlock::Ptr &bb);
    void translate_ret(const std::shared_ptr<ir::Ret> &ret, RVBasicBlock::Ptr bb);
    void translate_call(std::shared_ptr<ir::Call> call, RVBasicBlock::Ptr bb);
    void translate_alloca(std::shared_ptr<ir::Alloca> alloca, RVBasicBlock::Ptr bb);
    void translate_gep(std::shared_ptr<ir::Getelementptr> gep, RVBasicBlock::Ptr bb);
    void translate_conversion(std::shared_ptr<ir::Instruction> inst, RVBasicBlock::Ptr bb);
    void translate_memset(std::shared_ptr<ir::Memset> memset, RVBasicBlock::Ptr bb);

    // 新增：全局变量翻译
    void translate_global_variable(const std::shared_ptr<ir::GlobalVariable> &gv);

    // 新增：库函数处理
    void translate_lib_functions();

    // 乘法优化辅助函数
    std::vector<int> can_optimize(long long num);
    int get_shift(int temp);

    // 库函数判断辅助函数
    static bool is_no_stack_frame_lib_function(const std::string &func_name);

private:
    ir::Module *ir_module = nullptr;
    RVModule *rv_module = nullptr;
    RVFunction::Ptr current_function = nullptr;
    RVBasicBlock::Ptr current_basic_block = nullptr;

    // 映射表
    // 修改：value_to_operand 现在按后端基本块分组，每个后端基本块有自己的映射
    std::unordered_map<RVBasicBlock *, std::unordered_map<ir::Value *, RVOperand *>> value_to_operand_by_block;
    std::unordered_map<ir::BasicBlock *, std::string> bb_to_label = std::unordered_map<ir::BasicBlock *, std::string>();
    std::unordered_map<ir::Instruction *, std::vector<RVInstruction *>> predefines;

    // 新增：RVBasicBlock 到 ir::BasicBlock 的反向映射，用于访问支配关系信息
    std::unordered_map<RVBasicBlock *, ir::BasicBlock *> rvbb_to_irbb;

    // 新增：ir::BasicBlock 到 RVBasicBlock 的映射，用于查找支配块对应的后端基本块
    std::unordered_map<ir::BasicBlock *, RVBasicBlock *> bb_to_rvbb;

    // 分支跳转优化相关
    std::unordered_map<ir::Br *, std::vector<RVInstruction *>> br_to_branch =
        std::unordered_map<ir::Br *, std::vector<RVInstruction *>>();
    std::unordered_map<ir::Value *, int> ptr2offset = std::unordered_map<ir::Value *, int>();  // GEP 指针到偏移的映射

    // 新增：按支配关系查找操作数的方法
    RVOperand *find_operand_in_dominators(RVBasicBlock *current_block, ir::Value *ir_value);
    // 新增：全局查找操作数的方法，用于PHI和Move指令
    RVOperand *find_operand_globally(ir::Value *ir_value);
    void set_operand_for_block(RVBasicBlock *block, ir::Value *ir_value, RVOperand *operand);
    RVOperand *get_operand_for_block(RVBasicBlock *block, ir::Value *ir_value);
};

}  // namespace backend::riscv

#endif
