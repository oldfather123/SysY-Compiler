#ifndef __RISCV_INSTRUCTION_HPP__
#define __RISCV_INSTRUCTION_HPP__

#include <vector>
#include <unordered_map>
#include <string>
#include "riscv.hpp"
#include "ir_instruction.hpp"

enum RiscvInstID {
    ADD, SUB, MUL, DIV, REM,
    ADDI, SUBI,
    FADD, FSUB, FMUL, FDIV,
    SHL, LSHR, ASHR,
    SHLI, LSHRI, ASHRI,
    AND, OR, XOR,
    ANDI, ORI, XORI,
    ICMP, FCMP,
    FPTOSI, SITOFP,
    LI, MOV, FMV,
    LW, SW, FLW, FSW,
    PUSH, POP,
    CALL, RET,
    JMP, LA, BGT
};

extern unordered_map<RiscvInstID, string> inst_to_str;
extern unordered_map<ICmpOp, string> icmp_to_str;
extern unordered_map<ICmpOp, string> icmp_set_to_str;
extern unordered_map<ICmpOp, ICmpOp> icmp_transfer;
extern unordered_map<FCmpOp, string> fcmp_to_str;
extern unordered_map<FCmpOp, FCmpOp> fcmp_transfer;

// 由于汇编指令本身并不能作为指令的操作数，故为一个新类别
class RiscvInstruction {
public:
    RiscvInstID riid;               // 指令类型
    RiscvBasicBlock* parent;        // 所属基本块
    RiscvValue* ret_val;            // 结果寄存器或内存位置
    vector<RiscvValue*> operands;   // 操作数列表

    /// @brief 构造函数
    /// @param riid RISCV 指令类型
    /// @param num_ops 操作数数量
    /// @param rbb 所属基本块，默认为空，即不属于任何基本块
    RiscvInstruction(RiscvInstID riid, int num_ops, RiscvBasicBlock* rbb = nullptr);
    virtual string print() = 0;


    void set_op(int index, RiscvValue* rval);
    void remove_op(int index);
    RiscvValue* get_op(int index);
};

class RiscvBinaryInst: public RiscvInstruction {
public:
    bool is_word; // 是否为 32 位指令

    /// @brief 构造函数
    /// @param iid 指令类型
    /// @param rrval1 第一个操作数寄存器或内存位置
    /// @param rrval2 第二个操作数寄存器或内存位置
    /// @param target 结果寄存器或内存位置
    /// @param rbb 所属基本块
    /// @param is_word 是否为 32 位指令，默认为 false，即 64 位指令
    RiscvBinaryInst(RiscvInstID riid, RiscvValue* rrval1, RiscvValue* rrval2, RiscvValue* target, RiscvBasicBlock* rbb, bool is_word = false);
    string print() override;
};

class RiscvMoveInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param rval1 第一个操作数寄存器或内存位置
    /// @param rval2 第二个操作数寄存器或内存位置
    /// @param rbb 所属基本块
    RiscvMoveInst(RiscvValue* rval1, RiscvValue* rval2, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvLiInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param rval1 第一个操作数寄存器或内存位置
    /// @param imm 立即数
    /// @param rbb 所属基本块
    RiscvLiInst(RiscvValue* rval1, int imm, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvPushInst: public RiscvInstruction {
public:
    int offset; // 偏移量

    /// @brief 构造函数
    /// @param lists 要推送的值列表
    /// @param rbb 所属基本块
    /// @param offset 偏移量
    RiscvPushInst(vector<RiscvValue*>& lists, RiscvBasicBlock* rbb, int offset);
    string print() override;
};

class RiscvPopInst: public RiscvInstruction {
public:
    int offset; // 偏移量

    /// @brief 构造函数
    /// @param lists 要弹出的值列表
    /// @param rbb 所属基本块
    /// @param offset 偏移量
    RiscvPopInst(vector<RiscvValue*>& lists, RiscvBasicBlock* rbb, int offset);
    string print() override;
};

class RiscvCallInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param rfunc 调用的函数
    /// @param rbb 所属基本块
    RiscvCallInst(RiscvFunction* rfunc, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvReturnInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param rbb 所属基本块
    RiscvReturnInst(RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvLoadInst: public RiscvInstruction {
public:
    Type* type;
    int offset; // 地址偏移量

    /// @brief 构造函数
    /// @param type 数据类型
    /// @param target 目标寄存器
    /// @param base 基地址寄存器
    /// @param rbb 所属基本块
    /// @param offset 偏移量
    RiscvLoadInst(Type* type, RiscvValue* target, RiscvValue* base, RiscvBasicBlock* rbb, int offset = 0);
    string print() override;
};

class RiscvStoreInst: public RiscvInstruction {
public:
    Type* type; // 指明整型或者浮点型
    int offset; // 地址偏移量

    /// @brief 构造函数
    /// @param type 数据类型
    /// @param target 目标内存位置
    /// @param base 基地址寄存器
    /// @param rbb 所属基本块
    /// @param offset 偏移量
    RiscvStoreInst(Type* type, RiscvValue* target, RiscvValue* base, RiscvBasicBlock* rbb, int offset = 0);
    string print() override;
};

// 条件跳转指令
class RiscvICmpInst: public RiscvInstruction {
public:
    ICmpOp icmp_op; // 比较操作符

    /// @brief 构造函数
    /// @note 存在假分支的跳转，通常需要一个 true_bb 和一个 false_bb
    /// @param icmp_op 比较操作符
    /// @param rval1 第一个操作数
    /// @param rval2 第二个操作数
    /// @param true_bb 真分支基本块
    /// @param false_bb 假分支基本块
    /// @param rbb 所属基本块
    RiscvICmpInst(ICmpOp icmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvBasicBlock* true_bb, RiscvBasicBlock* false_bb, RiscvBasicBlock* rbb);
    string print() override;
};

// 比较设置指令：需要使用 icmp 的结果，不是直接跳转
class RiscvICmpSetInst: public RiscvInstruction {
public:
    ICmpOp icmp_op; // 比较操作符
    
    /// @brief 构造函数
    /// @note 该指令用于设置一个寄存器或内存位置的值为比较结果
    /// @param icmp_op 比较操作符
    /// @param rval1 第一个操作数
    /// @param rval2 第二个操作数
    /// @param target 目标寄存器或内存位置
    /// @param rbb 所属基本块
    RiscvICmpSetInst(ICmpOp icmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvValue* target, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvFCmpInst: public RiscvInstruction {
public:
    FCmpOp fcmp_op; // 比较符

    /// @brief 构造函数
    /// @param fcmp_op 比较操作符
    /// @param rval1 第一个操作数
    /// @param rval2 第二个操作数
    /// @param target_bb 目标基本块
    /// @param rbb 所属基本块
    RiscvFCmpInst(FCmpOp fcmp_op, RiscvValue* rval1, RiscvValue* rval2, RiscvValue* target_bb, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvFpToSiInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param source 源寄存器
    /// @param target 目标寄存器
    /// @param rbb 所属基本块
    RiscvFpToSiInst(RiscvValue* source, RiscvValue* target, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvSiToFpInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param source 源寄存器
    /// @param target 目标寄存器
    /// @param rbb 所属基本块
    RiscvSiToFpInst(RiscvValue* source, RiscvValue* target, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvLoadAddrInst: public RiscvInstruction {
public:
    string name; // 符号名称

    /// @brief 构造函数
    /// @param target 目标寄存器
    /// @param name 需要载入的符号名称
    /// @param rbb 所属基本块
    RiscvLoadAddrInst(RiscvValue* target, const string& name, RiscvBasicBlock* rbb);
    string print() override;
};

class RiscvBranchInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param cond 条件值
    /// @param true_bb 真分支
    /// @param false_bb 假分支
    /// @param rbb 
    RiscvBranchInst(RiscvValue* cond, RiscvBasicBlock* true_bb, RiscvBasicBlock* false_bb, RiscvBasicBlock* rbb);
    string print() override;
};

// 无条件跳转
class RiscvJmpInst: public RiscvInstruction {
public:
    /// @brief 构造函数
    /// @param target_bb 目标基本块
    /// @param rbb 所属基本块
    RiscvJmpInst(RiscvBasicBlock* target_bb, RiscvBasicBlock* rbb);
    string print() override;
};

#endif // __RISCV_INSTRUCTION_HPP__