#include "binaryinstr.h"

namespace IR
{
    BinaryInstruction::BinaryInstruction(BasicType *ty, BinaryOp op, Value *lhser, Value *rhser)
        : Instruction(ty, op)
    {
        setTotalUsers();
        if (!BasicType::checkType(lhser->getType(), rhser->getType()))
        {
            Error::Error(__PRETTY_FUNCTION__, "Binary operands must have the same type");
        }
        addUse(lhser);
        addUse(rhser);
    }
}