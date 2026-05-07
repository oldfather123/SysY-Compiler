#pragma once
#include "value.h"
#include "basictype.h"
#include "instruction.h"
#include "binaryinstr.h"
#include "unaryinstr.h"
#include "memoryinstr.h"
#include "otherinstr.h"
#include "terminatorinstr.h"
#include "basicblock.h"
#include "constantfolder.h"

namespace IR
{
    // czr: 这个就不用多说，folder和instruction测完这个基本就不会错
    struct IRBuilder
    {
        // TODO: 使用自己写的链表迭代器
        BasicBlock *BB;
        ConstantFolder *folder;

        IRBuilder()
        {
            BB = nullptr;
            folder = ConstantFolder::get();
        }

        // 关于插入点的函数
        void SetInsertPoint(BasicBlock *I)
        {
            BB = I;
        }

        void SetInsertPoint(BasicBlock *I, std::vector<Instruction *>::iterator IP)
        {
            BB = I;
        }

        BasicBlock *GetInsertBlock() { return BB; }

        // 创建指令的函数
        // 内存管理指令
        AllocaInstruction *CreateAlloca(BasicType *type, std::string name, bool is_const = false, const char *c = "");
        AllocaInstruction *CreateAlloca(BasicType *type, std::string name, Constant *init, bool is_const, const char *c = "");
        Value *CreateGEP(Value *base, std::vector<Value *> indices, const char *c = "");
        Value *CreateGEP(BasicType *type, Value *base, std::vector<Value *> indices, const char *c = "");
        StoreInstruction *CreateStore(Value *lval, Value *rval, const char *c = "");
        Value *CreateLoad(BasicType *ty, Value *op, const char *c = "");

        // 二元运算符
        Value *CreateAdd(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateSub(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateMul(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateDiv(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateRem(Value *lhs, Value *rhs, const char *c = "");

        Value *CreateFAdd(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFSub(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFMul(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFDiv(Value *lhs, Value *rhs, const char *c = "");

        // 一元运算符
        Value *CreateNeg(Value *val, const char *c = "");  // -val
        Value *CreateFNeg(Value *val, const char *c = ""); // -fval
        Value *CreateNot(Value *val, const char *c = "");  // !val
        Value *CreatePos(Value *val, const char *c = "");  // +val
        Value *CreateFPos(Value *val, const char *c = ""); // +fval

        // 位运算符
        Value *CreateAnd(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateOr(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateXor(Value *lhs, Value *rhs, const char *c = "");

        // 逻辑运算
        Value *CreateFEq(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFNe(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFLt(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFLe(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFGt(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateFGe(Value *lhs, Value *rhs, const char *c = "");

        Value *CreateEq(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateNe(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateLt(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateLe(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateGt(Value *lhs, Value *rhs, const char *c = "");
        Value *CreateGe(Value *lhs, Value *rhs, const char *c = "");

        // 函数调用
        CallInstruction *CreateCall(Function *callee, std::vector<Value *> args, const char *c = "");

        // 类型转换
        Value *CreateFPtoSI(Value *val, const char *c = "");
        Value *CreateSItoFP(Value *val, const char *c = "");

        // 控制流指令
        BranchInstruction *CreateCondBr(Value *cond, BasicBlock *then_block, BasicBlock *else_block, const char *c = "");
        BranchInstruction *CreateBr(BasicBlock *dest, const char *c = "");
        ReturnInstruction *CreateRet(Value *val, Function *func, const char *c = "");
        ReturnInstruction *CreateRetVoid(Function *func, const char *c = "");

        // 插入指令至插入点
        Value *Insert(Value *I);
        Value *InsertBack(Value *I);
        Value *InsertFront(Value *I);
    };

}
