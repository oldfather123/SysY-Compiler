#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "../ir/llvm_ir.h"
#include "./function_reg_alloc.h"
#include "./linear_scan_alloc.h"
#include "./riscv.h"

namespace riscv {

// class declaration
class riscvInstr;

class riscvBlock;
class riscvGlobalVariable;
class riscvFunction;
class riscvModule;

// constants
#define i32_bytes 4
#define i64_bytes 8
#define f32_bytes 4
#define f64_bytes 8
#define ptr_bytes 8
#define align_bytes_exp 4  // 2^4 = 16 B

// class define
class riscvBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<riscvInstr>> inst_list;

    std::string toString();
};

class riscvGlobalVariable {
public:
    std::string name;
    std::string size;
    std::vector<std::pair<int, std::string>> initializers;  // <.word/.zero, value_string>
};

class riscvFunction {
public:
    std::unique_ptr<riscvBlock> entry_block;
    std::vector<std::unique_ptr<riscvBlock>> basic_blocks;
    std::string name;
    int func_align = align_bytes_exp;
    int stack_align = align_bytes_exp;
    int data_align = align_bytes_exp;

    int Caller_size = 0; // 恢复a所用的内存 传递的参数放在最上面！！！！
    int Rasize = 0; //需要保存ra是 8 否则是 0 
    int Callee_size = 0; //恢复s所用的内存 
    int Regs2Save_size = 0; // Caller_size + Callee_size
    int SpilledVRegs_size = 0;    // Regs2Save_size + SpilledVRegs_size
    int total_alloca_size = 0;  // 跟踪所有alloca的总大小
    int current_alloca_offset = 0;  // 跟踪当前alloca的累积偏移
    int total_stack_size = 0;  // 最终计算出的总栈大小（已对齐）

    /// @Explain: our stack looks-like:
    /// |  ------------------  |
    /// |  ------  ra  ------  |  <--- do before generate this function
    /// |  ------------------  |
    /// |  callee-saved regs   |
    /// |  ------------------  |
    /// |  function params     |  <--- caller-save (parameters a0-a7, fa0-fa7)
    /// |  ------------------  |
    /// |  caller-save temps   |  <--- caller-save (temp regs t3-t6, ft3-ft11)
    /// |  ------------------  |
    /// |  spilled v-regs      |
    /// |  ------------------  |
    /// |  alloca space        |  <--- do while generating code
    /// |  ------------------  |

    // 寄存器分配结果 - 使用函数级分配结果
    std::set<riscv::reg> used_regs;
    int current_instruction_index = 0;  // 函数级别的指令计数器，用于活跃区间分析

    llvm_ir::FunctionRegAllocResult reg_alloc_result;
    
    // 函数参数信息（用于计算caller-saved寄存器）
    std::vector<llvm_ir::Value*> function_parameters;

    // alloca静态指针映射表：记录alloca产生的虚拟寄存器到栈地址的映射
    std::unordered_map<llvm_ir::Value*, int> alloca_ptr_to_offset;


    /**
     * Methods
     */
    std::string toString();

    // 设置寄存器分配结果
    void setRegAllocResult(const llvm_ir::FunctionRegAllocResult& result) { reg_alloc_result = result; }

    // 检查虚拟寄存器是否溢出到栈
    bool is_spilled(llvm_ir::Value* vreg);

    // 获取溢出变量的栈偏移
    int get_spill_offset(llvm_ir::Value* vreg);

    // 获取虚拟寄存器的分配信息
    const llvm_ir::RegAllocResult* get_allocation_info(llvm_ir::Value* vreg);

    // 检查是否为alloca产生的静态指针
    bool is_alloca_pointer(llvm_ir::Value* vreg);

    // 获取alloca指针对应的栈偏移
    int get_alloca_offset(llvm_ir::Value* vreg);

    // 记录alloca指针到栈偏移的映射
    void record_alloca_pointer(llvm_ir::Value* vreg, int stack_offset);

    // 生成load指令（从栈加载到寄存器）
    std::vector<std::unique_ptr<riscvInstr>> gen_load_from_stack(llvm_ir::Value* vreg, reg dest_reg);

    // 生成store指令（从寄存器存储到栈）
    std::vector<std::unique_ptr<riscvInstr>> gen_store_to_stack(llvm_ir::Value* vreg, reg src_reg);

    // 获取临时寄存器（仅用于use，不用于def）
    reg get_temp_reg_for_use(llvm_ir::Value* vreg, std::vector<std::unique_ptr<riscvInstr>>& post_insts);

    // 获取寄存器用于def（不能使用临时寄存器）
    reg get_reg_for_def(llvm_ir::Value* vreg, std::vector<std::unique_ptr<riscvInstr>>& post_insts);

    // 临时寄存器管理函数
    reg get_t_reg(bool is_float = false);

    // 初始化临时寄存器池
    void init_temp_reg_pool();

    int get_type_size(llvm_ir::Type type);
    int get_type_alignment(llvm_ir::Type type);

    // caller-saved寄存器管理
    void save_caller_saved_regs(std::vector<std::unique_ptr<riscvInstr>>& insts);
    void restore_caller_saved_regs(std::vector<std::unique_ptr<riscvInstr>>& insts);

    // 参数内存管理函数
    reg get_param_reg_value(llvm_ir::Value* param_reg, std::vector<std::unique_ptr<riscvInstr>>& load_insts);

private:
    // 临时寄存器池管理
    std::set<reg> available_temp_regs;
    std::set<reg> used_temp_regs;

};

class riscvModule {
public:
    std::vector<std::unique_ptr<riscvFunction>> func_list;
    std::vector<std::unique_ptr<riscvGlobalVariable>> global_vars;
    int func_id = 0;
    std::string toString();
};

class riscvInstr {
public:
    opcode op;
    reg rd;
    reg rs1;
    reg rs2;
    int imm;
    std::string label;

    std::string toString();
};

riscvModule* gen_module(llvm_ir::Module* llvm_program);
riscvFunction* gen_function(llvm_ir::Function* llvm_func);
riscvBlock* gen_block(llvm_ir::BasicBlock* llvm_block);
std::vector<std::unique_ptr<riscvInstr>> gen_instruction(llvm_ir::Instruction* llvm_inst);

// 具体指令生成函数
std::vector<std::unique_ptr<riscvInstr>> gen_binary_op(llvm_ir::BinaryOperator* bin_op);
std::vector<std::unique_ptr<riscvInstr>> gen_load(llvm_ir::LoadInst* load_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_store(llvm_ir::StoreInst* store_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_ret(llvm_ir::Instruction* ret_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_br(llvm_ir::Instruction* br_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_alloca(llvm_ir::AllocaInst* alloca_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_icmp(llvm_ir::ICmpInst* icmp_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_fcmp(llvm_ir::FCmpInst* fcmp_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_call(llvm_ir::Instruction* call_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_getelementptr(llvm_ir::GetElementPtrInst* gep_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_sitofp(llvm_ir::SIToFPInst* cast_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_fptosi(llvm_ir::FPToSIInst* cast_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_zext(llvm_ir::ZExtInst* zext_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_sext(llvm_ir::SExtInst* sext_inst);
std::vector<std::unique_ptr<riscvInstr>> gen_move(llvm_ir::MoveInst* move_inst);

}  // namespace riscv
