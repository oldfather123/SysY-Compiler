#include <cmath>
#include <unordered_map>
#include <string>
#include "ir_type.hpp"
#include "riscv_value.hpp"
#include "riscv_instruction.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"
#include "register.hpp"

// NOTE: 针对 32 位 RiscvBinaryInst，在 print() 函数中判断是否添加 W 后缀，即 ADDI -> ADDIW
unordered_map<RiscvInstID, string> inst_to_str = {
    {RiscvInstID::ADD, "ADD"},          {RiscvInstID::ADDI, "ADDI"},
    {RiscvInstID::SUB, "SUB"},          {RiscvInstID::SUBI, "SUBI"},
    {RiscvInstID::MUL, "MUL"},          {RiscvInstID::DIV, "DIV"},          {RiscvInstID::REM, "REM"},
    {RiscvInstID::FADD, "FADD.S"},      {RiscvInstID::FSUB, "FSUB.S"},
    {RiscvInstID::FMUL, "FMUL.S"},      {RiscvInstID::FDIV, "FDIV.S"},
    {RiscvInstID::SHL, "SLL"},          {RiscvInstID::LSHR, "SRL"},         {RiscvInstID::ASHR, "SRA"},
    {RiscvInstID::SHLI, "SLLI"},        {RiscvInstID::LSHRI, "SRLI"},       {RiscvInstID::ASHRI, "SRAI"},
    {RiscvInstID::AND, "AND"},          {RiscvInstID::OR, "OR"},            {RiscvInstID::XOR, "XOR"},
    {RiscvInstID::ANDI, "ANDI"},        {RiscvInstID::ORI, "ORI"},          {RiscvInstID::XORI, "XORI"},
    {RiscvInstID::ICMP, "ICMP"},        {RiscvInstID::FCMP, "FCMP.S"},
    {RiscvInstID::FPTOSI, "FCVT.W.S"},  {RiscvInstID::SITOFP, "FCVT.S.W"}, // 整型浮点型转换
    {RiscvInstID::LI, "LI"},            {RiscvInstID::MOV, "MV"},           {RiscvInstID::FMV, "FMV.S"}, // 加载立即数和寄存器移动
    {RiscvInstID::LW, "LW"},            {RiscvInstID::SW, "SW"}, // 存取
    {RiscvInstID::FLW, "FLW"},          {RiscvInstID::FSW, "FSW"}, // 浮点存取
    {RiscvInstID::PUSH, "PUSH"},        {RiscvInstID::POP, "POP"}, // 压栈和出栈
    {RiscvInstID::CALL, "CALL"},        {RiscvInstID::RET, "RET"}, // 函数调用和返回
    {RiscvInstID::JMP, "JMP"},          {RiscvInstID::LA, "LA"}, // 跳转和加载地址
    {RiscvInstID::BGT, "BGT"} // 分支跳转
};

// NOTE: RISC-V 不支持 GT 和 LE, 需要转换
unordered_map<ICmpOp, string> icmp_to_str = {
    {ICmpOp::EQ, "BEQ"}, {ICmpOp::NE, "BNE"}, // ==, !=
    {ICmpOp::GE, "BGE"}, {ICmpOp::LT, "BLT"}, // >=, <
};

// NOTE: RISC-V 不支持 GT, GE 和 LE 的结果保存，需要转换
unordered_map<ICmpOp, string> icmp_set_to_str = {
    {ICmpOp::EQ, "SEQZ"}, {ICmpOp::NE, "SNEZ"}, {ICmpOp::LT, "SLT"}, // ==, !=, <
    {ICmpOp::GE, "SLT"} // NOTE: 同时注意 a >= b 不需要进行操作数的互换，直接对 a < b 的结果取反即可
};

unordered_map<ICmpOp, ICmpOp> icmp_transfer = {
    {ICmpOp::GT, ICmpOp::LT}, // a > b 转换为 b < a
    {ICmpOp::LE, ICmpOp::GE}  // a <= b 转换为 b >= a
};

// NOTE: 同样，针对不支持的 FGT 和 FNE 进行转换
unordered_map<FCmpOp, string> fcmp_to_str = {
    {FCmpOp::FEQ, "FEQ.S"}, {FCmpOp::FLT, "FLT.S"}, {FCmpOp::FLE, "FLE.S"}
    , {FCmpOp::FNE, "FEQ.S"} // 不等取反
};

unordered_map<FCmpOp, FCmpOp> fcmp_transfer = {
    {FCmpOp::FGT, FCmpOp::FLT}, // a > b 转换为 b < a
    {FCmpOp::FGE, FCmpOp::FLE} // a >= b 转换为 b <= a
};

// --------------------------------  构造函数  --------------------------------
RiscvInstruction::RiscvInstruction(RiscvInstID riid, int num_ops, RiscvBasicBlock* rbb)
    : riid(riid), parent(rbb), ret_val(nullptr) {
    operands.resize(num_ops);
}

RiscvBinaryInst::RiscvBinaryInst(RiscvInstID riid, RiscvValue* rval1, RiscvValue* rval2, RiscvValue* target, RiscvBasicBlock* rbb, bool is_word)
    : RiscvInstruction(riid, 2, rbb), is_word(is_word) {
    set_op(0, rval1);
    set_op(1, rval2);
    ret_val = target;
}

RiscvMoveInst::RiscvMoveInst(RiscvValue* rval1, RiscvValue* rval2, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::MOV, 2, rbb) {
    set_op(0, rval1);
    set_op(1, rval2);
    // if(flag) {
    //     parent->add_rinst_back(this);
    // }
}

RiscvLiInst::RiscvLiInst(RiscvValue* rval1, int imm, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::LI, 2, rbb) {
    set_op(0, rval1);
    set_op(1, new RiscvConst(imm));
    // if(flag) {
    //     parent->add_rinst_back(this);
    // }
}

RiscvPushInst::RiscvPushInst(vector<RiscvValue*>& lists, RiscvBasicBlock* rbb, int offset)
    : RiscvInstruction(RiscvInstID::PUSH, lists.size(), rbb), offset(offset) {
    for(int i = 0; i < lists.size(); ++i) {
        set_op(i, lists[i]);
    }
}

RiscvPopInst::RiscvPopInst(vector<RiscvValue*>& lists, RiscvBasicBlock* rbb, int offset)
    : RiscvInstruction(RiscvInstID::POP, lists.size(), rbb), offset(offset) {
    for(int i = 0; i < lists.size(); ++i) {
        set_op(i, lists[i]);
    }
}

RiscvCallInst::RiscvCallInst(RiscvFunction* func, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::CALL, 1, rbb) {
    set_op(0, func);
}

RiscvReturnInst::RiscvReturnInst(RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::RET, 0, rbb) {
}

RiscvLoadInst::RiscvLoadInst(Type* type, RiscvValue* dest, RiscvValue* target, RiscvBasicBlock* rbb, int offset)
    : RiscvInstruction(RiscvInstID::LW, 2, rbb), offset(offset), type(type) {
    set_op(0, dest);
    set_op(1, target);
    if(!target) {
        cerr << "[Error] Invalid load instruction: " << print() << "\n";
        exit(1);
    }
    // TODO: 跟随 type->tid 设置 this->riid: LW/FLW，store 同理
}

RiscvStoreInst::RiscvStoreInst(Type* type, RiscvValue* source, RiscvValue* target, RiscvBasicBlock* rbb, int offset)
    : RiscvInstruction(RiscvInstID::SW, 2, rbb), offset(offset), type(type) {
    set_op(0, source);
    set_op(1, target);
    if(!source->is_reg()) {
        cerr << "[Error] Invalid store instruction: " << print() << "\n";
        exit(1);
    }
}

RiscvICmpInst::RiscvICmpInst(ICmpOp icmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvBasicBlock* true_bb, RiscvBasicBlock* false_bb, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::ICMP, 4, rbb), icmp_op(icmp_op) {
    set_op(0, rval1);
    set_op(1, rval2);
    set_op(2, true_bb);
    set_op(3, false_bb);
}

RiscvICmpSetInst::RiscvICmpSetInst(ICmpOp icmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvValue* target, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::ICMP, 2, rbb), icmp_op(icmp_op) {
    set_op(0, rval1);
    set_op(1, rval2);
    ret_val = target;
}

RiscvFCmpInst::RiscvFCmpInst(FCmpOp fcmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvValue* target_bb, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::FCMP, 2, rbb), fcmp_op(fcmp_op) {
    set_op(0, rval1);
    set_op(1, rval2);
    ret_val = target_bb;
}

RiscvFpToSiInst::RiscvFpToSiInst(RiscvValue* source, RiscvValue* target, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::FPTOSI, 2, rbb) {
    set_op(0, source);
    set_op(1, target);
}

RiscvSiToFpInst::RiscvSiToFpInst(RiscvValue* source, RiscvValue* target, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::SITOFP, 2, rbb) {
    set_op(0, source);
    set_op(1, target);
}

RiscvLoadAddrInst::RiscvLoadAddrInst(RiscvValue* target, const string& name, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::LA, 1, rbb), name(name) {
    set_op(0, target);
}

RiscvBranchInst::RiscvBranchInst(RiscvValue* cond, RiscvBasicBlock* true_bb, RiscvBasicBlock* false_bb, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::BGT, 3, rbb) {
    set_op(0, cond);
    set_op(1, true_bb);
    set_op(2, false_bb);
}

RiscvJmpInst::RiscvJmpInst(RiscvBasicBlock* target_bb, RiscvBasicBlock* rbb)
    : RiscvInstruction(RiscvInstID::JMP, 1, rbb) {
    set_op(0, target_bb);
}

// --------------------------------  功能函数  --------------------------------

void RiscvInstruction::set_op(int index, RiscvValue* val) {
    if (index < 0 || index >= operands.size()) {
        cerr << "[Error] Index out of bounds when setting operand in instruction.\n";
        exit(1);
    }
    operands[index] = val;
}

void RiscvInstruction::remove_op(int index) {
    if (index < 0 || index >= operands.size()) {
        cerr << "[Error] Index out of bounds when removing operand in instruction.\n";
        exit(1);
    }
    operands[index] = nullptr; // Set to nullptr to indicate removal
}

RiscvValue* RiscvInstruction::get_op(int index) {
    if (index < 0 || index >= operands.size()) {
        cerr << "[Error] Index out of bounds when getting operand in instruction.\n";
        exit(1);
    }
    return operands[index];    
}