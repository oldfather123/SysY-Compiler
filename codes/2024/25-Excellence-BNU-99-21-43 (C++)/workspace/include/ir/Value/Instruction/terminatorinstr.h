
#pragma once
#include "instruction.h"
#include "basicblock.h"
#include "error.h"
#include "globalvalue.h"
#include "voidtype.h"
namespace IR
{
    // czr: 懂得都懂，测一下。
    // operands: [condition, trueBlock, falseBlock]
    struct BranchInstruction : public Instruction
    {
        bool isCond = 0;
        bool isConditional() { return isCond; };
        bool isUnconditional() { return !isCond; };
        BranchInstruction(Value *condition, BasicBlock *trueBlock, BasicBlock *falseBlock);

        BranchInstruction(BasicBlock *thenBlock); // unconditional branch

        void setCondition(Value *condition);

        void setTrueBlock(BasicBlock *trueBlock);

        void setFalseBlock(BasicBlock *falseBlock);

        void setUnconditionalBlock(BasicBlock *thenBlock);

        Value *getCondition()
        {
            if (!isCond)
                Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
            return operands[0];
        }

        BasicBlock *getTrueBlock()
        {
            if (!isCond)
                Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
            return static_cast<BasicBlock *>(operands[1]);
        }

        BasicBlock *getFalseBlock()
        {
            if (!isCond)
                Error::Error(__PRETTY_FUNCTION__, "Branch is not conditional");
            return static_cast<BasicBlock *>(operands[2]);
        }

        BasicBlock *getUnconditionalBlock()
        {
            if (isCond)
                Error::Error(__PRETTY_FUNCTION__, "Branch is not unconditional");
            return static_cast<BasicBlock *>(operands[0]);
        }

        void emitIR(std::ostream &os) override
        {
            if (isCond)
            {
                os << "if" << " " << operands[0]->getTypeName() << " " << operands[0]->getIRName()
                   << ", goto label %" << operands[1]->getIRName()
                   << ", then label %" << operands[2]->getIRName() << std::endl;
            }
            else
            {
                os << "goto" << " label %" << operands[0]->getIRName() << std::endl;
            }
        }

        static BranchInstruction *createCondBr(Value *condition, BasicBlock *trueBlock, BasicBlock *falseBlock)
        {
            assert(condition->getType()->isInt32());
            return new BranchInstruction(condition, trueBlock, falseBlock);
        }

        static BranchInstruction *createBr(BasicBlock *thenBlock)
        {
            return new BranchInstruction(thenBlock);
        }
    };

    struct ReturnInstruction : public Instruction
    {
        Function *parentFunc;
        ReturnInstruction(Value *retVal, Function *func);

        ReturnInstruction(Function *func);

        bool isVoidReturn()
        {
            return operands.size() == 0;
        }

        void emitIR(std::ostream &os) override
        {
            os << getOpStr();
            if (!isVoidReturn())
                os << " " << operands[0]->getTypeName() << " " << operands[0]->getIRName();
            else
                os << " void";
            os << std::endl;
        }

        static ReturnInstruction *create(Value *retVal, Function *func)
        {
            if (!BasicType::checkType(func->getReturnType(), retVal->getType()))
                Error::Error(__PRETTY_FUNCTION__, "Function return type must not be void");
            return new ReturnInstruction(retVal, func);
        }

        static ReturnInstruction *createVoid(Function *func)
        {
            if (!BasicType::checkType(func->getReturnType(), BasicType::getVoidType()))
                Error::Error(__PRETTY_FUNCTION__, "Function return type must be void");
            return new ReturnInstruction(func);
        }
    };

}