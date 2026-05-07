#ifndef __IR_BUILDER_HPP__
#define __IR_BUILDER_HPP__

#include <vector>
#include "ir.hpp"

class IRBuilder {
public:
    Module* module;
    BasicBlock* cur_bb;

    /// @brief 构造函数
    /// @param module 所属模块
    /// @param cur_bb 当前基本块
    IRBuilder(Module* module, BasicBlock* cur_bb);

    /// @brief 二元运算指令
    /// @param val1 左操作数
    /// @param val2 右操作数
    /// @return 根据类型创建的指令
    BinaryInst* create_add(Value* val1, Value* val2);
    BinaryInst* create_sub(Value* val1, Value* val2);
    BinaryInst* create_mul(Value* val1, Value* val2);
    BinaryInst* create_div(Value* val1, Value* val2);
    BinaryInst* create_rem(Value* val1, Value* val2);
    BinaryInst* create_fadd(Value* val1, Value* val2);
    BinaryInst* create_fsub(Value* val1, Value* val2);
    BinaryInst* create_fmul(Value* val1, Value* val2);
    BinaryInst* create_fdiv(Value* val1, Value* val2);
    BinaryInst* create_shl(Value* val1, Value* val2);
    BinaryInst* create_ashr(Value* val1, Value* val2);
    BinaryInst* create_lshr(Value* val1, Value* val2);

    /// @brief 比较指令
    /// @param val1 左操作数
    /// @param val2 右操作数
    /// @return 根据类型创建的指令
    ICmpInst* create_icmp_eq(Value* val1, Value* val2);
    ICmpInst* create_icmp_ne(Value* val1, Value* val2);
    ICmpInst* create_icmp_gt(Value* val1, Value* val2);
    ICmpInst* create_icmp_ge(Value* val1, Value* val2);
    ICmpInst* create_icmp_lt(Value* val1, Value* val2);
    ICmpInst* create_icmp_le(Value* val1, Value* val2);
    FCmpInst* create_fcmp_eq(Value* val1, Value* val2);
    FCmpInst* create_fcmp_ne(Value* val1, Value* val2);
    FCmpInst* create_fcmp_gt(Value* val1, Value* val2);
    FCmpInst* create_fcmp_ge(Value* val1, Value* val2);
    FCmpInst* create_fcmp_lt(Value* val1, Value* val2);
    FCmpInst* create_fcmp_le(Value* val1, Value* val2);

    /// @brief 栈上分配指令 alloca
    /// @param type 需要分配的类型
    /// @return 创建的 alloca 指令
    AllocaInst* create_alloca(Type* type);

    /// @brief 获取元素指针指令
    /// @param ptr 指向的指针
    /// @param idxs 索引列表
    /// @return 创建的 getelementptr 指令
    GetElementPtrInst* create_gep(Value* ptr, vector<Value*> idxs);

    /// @brief 创建加载指令
    /// @param ptr 指向的指针
    /// @return 创建的 load 指令
    LoadInst* create_load(Value* ptr);

    /// @brief 创建存储指令
    /// @param val 待存储的值
    /// @param ptr 指向的指针
    /// @return 创建的 store 指令
    StoreInst* create_store(Value* val, Value* ptr);

    /// @brief 一元运算指令：本编译器只包含 zext、fptosi、sitofp 和 bitcast 四种类型转换指令
    /// @param val 操作数
    /// @param target_type 目标类型（类型转换指令）
    /// @return 创建的指令
    UnaryInst* create_zext(Value* val, Type* target_type);
    UnaryInst* create_fptosi(Value* val, Type* target_type);
    UnaryInst* create_sitofp(Value* val, Type* target_type);
    UnaryInst* create_bitcast(Value* val, Type* target_type);

    /// @brief 函数调用指令
    /// @param func 调用的函数
    /// @param args 函数参数列表
    /// @return 创建的 call 指令
    CallInst* create_call(Function* func, vector<Value*> args);

    /// @brief 无条件分支指令
    /// @param target_bb 跳转目标基本块
    /// @return 创建的 br 指令
    BranchInst* create_uncond_br(BasicBlock* target_bb);

    /// @brief 有条件分支指令
    /// @param cond 条件
    /// @param true_bb 真分支目标基本块
    /// @param false_bb 假分支目标基本块
    /// @return 创建的 br 指令
    BranchInst* create_cond_br(Value* cond, BasicBlock* true_bb, BasicBlock* false_bb);

    /// @brief 返回指令
    /// @param val 返回值，可为空，即 ret void
    /// @return 创建的 ret 指令
    ReturnInst* create_ret(Value* val = nullptr);
};

#endif // __IR_BUILDER_HPP__