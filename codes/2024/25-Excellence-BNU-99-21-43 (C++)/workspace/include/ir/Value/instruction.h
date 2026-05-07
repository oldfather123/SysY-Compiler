#pragma once
#include "user.h"
#include "listnode.h"
#include "list.h"

namespace IR
{
    // czr: 好像没有什么好测的。因为不是final类
    struct BasicBlock;
    struct Instruction
        : public User,
          public ListNode
    {
        virtual ~Instruction() = default;

        BasicBlock *parent = nullptr;

        Instruction(BasicType *type) : User(type, Value::InstructionVal), ListNode(1) {}

        Instruction(BasicType *type, const unsigned int opCode) : User(type, Value::InstructionVal + opCode), ListNode(1) {}

        BasicBlock *getParentBB() const { return parent; }

        bool isUseless();

        Value *getOperand(unsigned int i) const override { return operands[i]; }

        std::string getIRName() const override
        {
            return "@" + std::to_string(number);
        };
        // 设计参考 LLVM 指令集

        enum BinaryOp
        {
            // 二元运算符
            BinaryBegin = 3,
            Add = 3,
            Sub = 4,
            Mul = 5,
            Div = 6,
            Rem = 7,

            FAdd = 8,
            FSub = 9,
            FMul = 10,
            FDiv = 11,
            FRem = 12,
            // 位运算符
            And = 13,
            Or = 14,
            Xor = 15,
            BinaryEnd = 16,
        };

        enum UnaryOp
        {
            // 一元运算符
            UnaryBegin = 16,
            Neg = 16,
            FNeg = 17,
            Not = 18,
            UnaryEnd = 19,
        };

        enum MemoryOp
        {
            // 内存操作
            MemoryBegin = 19,
            Alloca = 19,
            Load = 20,
            Store = 21,
            GEP = 22,
            MemoryEnd = 23,
        };

        enum CastOp
        {
            // 类型转换
            CastBegin = 23,
            FPtoSI = 23,
            SItoFP = 24,
            CastEnd = 25,
        };

        enum CmpOp
        {
            // 比较操作
            CmpBegin = 25,
            ICmp = 25,
            Ne = 26,
            Lt = 27,
            Le = 28,
            Gt = 29,
            Ge = 30,
            Eq = 31,
            FCmp = 32,
            FNe = 33,
            FLt = 34,
            FLe = 35,
            FGt = 36,
            FGe = 37,
            FEq = 38,
            CmpEnd = 39,
        };

        enum OtherOp
        {
            // 其他操作
            OtherOp = 39,
            Call = 40,
            Phi = 41,
            Select = 42,
            OtherEnd = 43,
        };

        enum TerminatorOp
        {
            TerminatorBegin = 43,
            Return = 43,
            BR = 44,
            InderectBr = 45,
            TerminatorEnd = 46
        };

        unsigned int getOpcode()
        {
            return subClassID - Value::InstructionVal;
        }

        std::string getOpStr();

        void waste() override;

        bool isTerminator()
        {
            return getOpcode() >= TerminatorBegin && getOpcode() <= TerminatorEnd;
        }

    private:
        friend struct List<Instruction *>;
        void removeNode() override;
    };
}