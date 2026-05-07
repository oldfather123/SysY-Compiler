#include "terminatorinstr.h"
#include "voidtype.h"

namespace IR
{
    BranchInstruction::BranchInstruction(Value *condition, BasicBlock *trueBlock, BasicBlock *falseBlock)
        : Instruction(BasicType::getVoidType(), TerminatorOp::BR)
    {
        setTotalUsers();
        addUse(condition);
        addUse(trueBlock);
        addUse(falseBlock);
        isCond = true;
    }

    BranchInstruction::BranchInstruction(BasicBlock *thenBlock) // unconditional branch
        : Instruction(BasicType::getVoidType(), TerminatorOp::BR)
    {
        setTotalUsers();
        addUse(thenBlock);
        isCond = false;
    }

    void BranchInstruction::setCondition(Value *condition)
    {
        if (!isCond)
            Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
        auto use = uses[0];
        use->setValue(condition);
        assert(operands[0] == condition);
    }

    void BranchInstruction::setTrueBlock(BasicBlock *trueBlock)
    {
        if (!isCond)
            Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
        auto use = uses[1];
        use->setValue(trueBlock);
        assert(operands[1] == trueBlock);
    }

    void BranchInstruction::setFalseBlock(BasicBlock *falseBlock)
    {
        if (!isCond)
            Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
        auto use = uses[2];
        use->setValue(falseBlock);
        assert(operands[2] == falseBlock);
    }

    void BranchInstruction::setUnconditionalBlock(BasicBlock *thenBlock)
    {
        if (isCond)
            Error::Error(__PRETTY_FUNCTION__, "Branch is not unconditional");
        auto use = uses[0];
        use->setValue(thenBlock);
        assert(operands[0] == thenBlock);
    }

    ReturnInstruction::ReturnInstruction(Value *retVal, Function *func)
        : Instruction(BasicType::getVoidType(), TerminatorOp::Return)
    {
        setTotalUsers();
        addUse(retVal);
        parentFunc = func;
    }

    ReturnInstruction::ReturnInstruction(Function *func)
        : Instruction(BasicType::getVoidType(), TerminatorOp::Return)
    {
        setTotalUsers();
        parentFunc = func;
    }

}