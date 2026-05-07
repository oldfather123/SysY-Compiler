#pragma once
#include "instruction.h"
#include "error.h"

#define HANDLE_BINARY_CREATE(op)                                                \
    static BinaryInstruction *create##op(BasicType *ty, Value *lhs, Value *rhs) \
    {                                                                           \
        return new BinaryInstruction(ty, BinaryOp::op, lhs, rhs);               \
    }

namespace IR
{
    struct BinaryInstruction : public Instruction
    {
        BinaryInstruction(BasicType *ty, BinaryOp op, Value *lhser, Value *rhser);
        // x = op type op1 op2
        void emitIR(std::ostream &os)
        {
            os << getIRName() + " = " + getOpStr() + " " + type->getTypeName() + " " + getOperand(0)->getIRName() + " " + getOperand(1)->getIRName() + "\n";
        }

        static BinaryInstruction *create(BasicType *ty, BinaryOp op, Value *lhs, Value *rhs)
        {
            return new BinaryInstruction(ty, op, lhs, rhs);
        }

        // czr: 全是静态函数，用于创建BinaryInstruction对象

        HANDLE_BINARY_CREATE(Add)
        HANDLE_BINARY_CREATE(Sub)
        HANDLE_BINARY_CREATE(Mul)
        HANDLE_BINARY_CREATE(Div)
        HANDLE_BINARY_CREATE(Rem)
        HANDLE_BINARY_CREATE(And)
        HANDLE_BINARY_CREATE(Or)
        HANDLE_BINARY_CREATE(Xor)
        HANDLE_BINARY_CREATE(FAdd)
        HANDLE_BINARY_CREATE(FSub)
        HANDLE_BINARY_CREATE(FMul)
        HANDLE_BINARY_CREATE(FDiv)
        HANDLE_BINARY_CREATE(FRem)
    };
}

#undef HANDLE_BINARY_CREATE
