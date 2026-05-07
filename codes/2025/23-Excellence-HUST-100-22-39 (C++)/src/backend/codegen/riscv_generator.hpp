#ifndef __RISCVGENERATOR_HPP__
#define __RISCVGENERATOR_HPP__

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "riscv.hpp"

class RiscvGenerator {
private:
    Module* m;
    int label_count; // 用于生成唯一的标签
    unordered_map<BasicBlock*, RiscvBasicBlock*> bb_to_rbb;
    unordered_map<Function*, RiscvFunction*> func_to_rfunc;
    unordered_set<Value*> allocated_val;    // 已分配到栈上的 Value
    int int_param_count;    // 整型参数计数
    int float_param_count;  // 浮点型参数计数
    int param_offset;       // 参数偏移量，用于计算栈帧大小

    string to_label(int index);
    RiscvBasicBlock* get_rbb_from_bb(BasicBlock* bb = nullptr);
    RiscvFunction* get_rfunc_from_func(Function* func);
    RiscvFunction* create_lib_func(Function* func);
    void init_ret_inst(RegAlloc* reg_alloc, RiscvInstruction* ret_inst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);

    // NOTE: 下面的方法是将 IR 指令转换为 RISC-V 指令的核心方法，但不是创建汇编指令的唯一途径
    RiscvBinaryInst* translate_binary_inst(RegAlloc* reg_alloc, BinaryInst* binary_inst, RiscvBasicBlock* rbb);
    RiscvCallInst* translate_call_inst(RegAlloc* reg_alloc, CallInst* call_inst, RiscvBasicBlock* rbb);
    RiscvReturnInst* translate_ret_inst(RegAlloc* reg_alloc, ReturnInst* ret_inst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    vector<RiscvInstruction*> translate_load_inst(RegAlloc* reg_alloc, LoadInst* load_inst, RiscvBasicBlock* rbb);
    vector<RiscvInstruction*> translate_store_inst(RegAlloc* reg_alloc, StoreInst* store_inst, RiscvBasicBlock* rbb);
    // NOTE: 指令选择阶段识别 icmp 指令是否被且仅被 cond-br 指令使用且二者相邻，如果是的话直接生成 icmp 指令，否则生成 icmp_set 指令
    RiscvInstruction* translate_icmp_inst(RegAlloc* reg_alloc, ICmpInst* icmp_inst, BranchInst* br_inst, RiscvBasicBlock* rbb);
    RiscvInstruction* translate_icmp_set_inst(RegAlloc* reg_alloc, ICmpInst* icmp_inst, RiscvBasicBlock* rbb);
    RiscvInstruction* translate_fcmp_inst(RegAlloc* reg_alloc, FCmpInst* fcmp_inst, RiscvBasicBlock* rbb);
    RiscvFpToSiInst* translate_fptosi_inst(RegAlloc* reg_alloc, UnaryInst* fptosi_inst, RiscvBasicBlock* rbb);
    RiscvSiToFpInst* translate_sitofp_inst(RegAlloc* reg_alloc, UnaryInst* sitofp_inst, RiscvBasicBlock* rbb);
    RiscvInstruction* translate_br_inst(RegAlloc* reg_alloc, BranchInst* br_inst, RiscvBasicBlock* rbb);
    // vector<RiscvInstruction*> translate_br_inst(RegAlloc* reg_alloc, BranchInst* br_inst, RiscvBasicBlock* rbb);
    RiscvInstruction* translate_gep_inst(RegAlloc* reg_alloc, GetElementPtrInst* gep_inst, RiscvBasicBlock* rbb);
    void translate_phi_inst(RegAlloc* reg_alloc, BasicBlock* bb);
    RiscvBasicBlock* transfer_rbb_from_bb(BasicBlock* bb, RiscvFunction* rfunc);

    void inst_emit(RiscvFunction* rfunc);
    void handle_binary(RiscvBinaryInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handle_load(RiscvLoadInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handle_store(RiscvStoreInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handel_push(RiscvPushInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handle_pop(RiscvPopInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handle_branch(RiscvBranchInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);
    void handle_icmp(RiscvICmpInst* rinst, RiscvBasicBlock* rbb, RiscvFunction* rfunc);

    void optimize(RiscvFunction* rfunc);

    void store_on_stack(Value* val, RiscvFunction* rfunc);

    void generate_data_section(ostream& out);
    void generate_code_section(ostream& out);
public:
    RiscvGenerator(Module* m);
    // TODO: 将后端的代码段生成部分拆分成三个阶段：translate, emit, print，现阶段将 emit 融入了 print 阶段
    void generate(ostream& out); // 生成 RISC-V 代码
};

#endif // __RISCVGENERATOR_HPP__