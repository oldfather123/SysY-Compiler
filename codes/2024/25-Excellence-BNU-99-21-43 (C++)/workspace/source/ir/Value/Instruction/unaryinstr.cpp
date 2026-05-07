#include "unaryinstr.h"

namespace IR
{

    UnaryInstruction::UnaryInstruction(BasicType *type, const unsigned int x, Value *op) : Instruction(type, x)
    {
        setTotalUsers();
        addUse(op);
    }
    UnaryInstruction::UnaryInstruction(BasicType *type, UnaryOp x, Value *op) : Instruction(type, x)
    {
        setTotalUsers();
        addUse(op);
    }

    CastInstruction::CastInstruction(BasicType *type, CastOp x, Value *op) : UnaryInstruction(type, x, op) {}
    CastInstruction::CastInstruction(BasicType *type, const unsigned int x, Value *op) : UnaryInstruction(type, x, op) {}
}