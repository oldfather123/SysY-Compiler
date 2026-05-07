#pragma once
#include "instruction.h"
#include "basictype.h"
#include "int32type.h"
#define HANDLE_UNARY_CREATE(op)                                   \
    static UnaryInstruction *create##op(BasicType *ty, Value *op) \
    {                                                             \
        return new UnaryInstruction(ty, UnaryOp::op, op);         \
    }
#define HANDLE_CAST_CREATE(op)                                   \
    static CastInstruction *create##op(BasicType *ty, Value *op) \
    {                                                            \
        return new CastInstruction(ty, CastOp::op, op);          \
    }

namespace IR
{
    // czr: 和BinaryInstruction类似，只是这里是一元操作符
    struct UnaryInstruction : public Instruction
    {
        UnaryInstruction(BasicType *type, const unsigned int x, Value *op);
        UnaryInstruction(BasicType *type, UnaryOp x, Value *op);

        void emitIR(std::ostream &os) override
        {
            os << getIRName() << " = " << getOpStr() << " " << getOperand(0)->getIRName() << std::endl;
        }

        static UnaryInstruction *create(BasicType *type, UnaryOp x, Value *op)
        {
            return new UnaryInstruction(type, x, op);
        }

        HANDLE_UNARY_CREATE(Neg)
        HANDLE_UNARY_CREATE(FNeg)
        static UnaryInstruction *createNot(BasicType *ty, Value *op)
        {
            return UnaryInstruction::create(BasicType::getInt32Type(), UnaryOp::Not, op);
        }
    };

    struct CastInstruction : public UnaryInstruction
    {
        CastInstruction(BasicType *type, CastOp x, Value *op);
        CastInstruction(BasicType *type, const unsigned int x, Value *op);

        static CastInstruction *create(BasicType *type, CastOp x, Value *op)
        {
            return new CastInstruction(type, x, op);
        }
        HANDLE_CAST_CREATE(FPtoSI)
        HANDLE_CAST_CREATE(SItoFP)
    };
}

#undef HANDLE_UNARY_CREATE
#undef HANDLE_CAST_CREATE
