#include "constantfoldpass.h"

#define FOLD_BINARY(OPNAME, TYPE, OPTYPE, OP)                                                                             \
    if (binInstr->getOpcode() == Instruction::OPNAME)                                                                     \
    {                                                                                                                     \
        assert(binInstr->getType()->is##TYPE());                                                                          \
        assert(lhs->isConstant##OPTYPE() && rhs->isConstant##OPTYPE());                                                   \
        auto result = static_cast<Constant##OPTYPE *>(lhs)->getValue() OP static_cast<Constant##TYPE *>(rhs)->getValue(); \
        binInstr->replaceAllUsageTo(Constant##TYPE::get(result));                                                         \
        binInstr->waste();                                                                                                \
    }

#define FOLD_UNARY(OPNAME, TYPE, OPTYPE, OP)                                    \
    if (unaryInstr->getOpcode() == Instruction::OPNAME)                         \
    {                                                                           \
        assert(unaryInstr->getType()->is##TYPE());                              \
        assert(operand->isConstant##OPTYPE());                                  \
        auto result = OP(static_cast<Constant##OPTYPE *>(operand)->getValue()); \
        unaryInstr->replaceAllUsageTo(Constant##TYPE::get(result));             \
        unaryInstr->waste();                                                    \
    }

namespace IR
{
    bool ConstantFoldPass::isConstant(Value *val)
    {
        if (val->isConstantInt32())
        {
            return true;
        }
        else if (val->isConstantFloat())
        {
            return true;
        }
        return false;
    }

    void ConstantFoldPass::foldInstruction(Instruction *instr)
    {
        if (instr->getOpcode() >= Instruction::BinaryBegin && instr->getOpcode() < Instruction::BinaryEnd)
        {
            BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
            Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
            if (!isConstant(lhs) || !isConstant(rhs))
                return;
            FOLD_BINARY(Add, Int32, Int32, +)
            FOLD_BINARY(Sub, Int32, Int32, -)
            FOLD_BINARY(Mul, Int32, Int32, *)
            FOLD_BINARY(Div, Int32, Int32, /)
            FOLD_BINARY(Rem, Int32, Int32, %)
            FOLD_BINARY(FAdd, Float, Float, +)
            FOLD_BINARY(FSub, Float, Float, -)
            FOLD_BINARY(FMul, Float, Float, *)
            FOLD_BINARY(FDiv, Float, Float, /)
            FOLD_BINARY(And, Int32, Int32, &)
            FOLD_BINARY(Or, Int32, Int32, |)
            FOLD_BINARY(Xor, Int32, Int32, ^)
        }
        else if (instr->getOpcode() >= Instruction::UnaryBegin && instr->getOpcode() < Instruction::UnaryEnd)
        {
            UnaryInstruction *unaryInstr = static_cast<UnaryInstruction *>(instr);
            Value *operand = unaryInstr->getOperand(0);
            if (!isConstant(operand))
                return;
            FOLD_UNARY(Neg, Int32, Int32, -)
            FOLD_UNARY(FNeg, Float, Float, -)
            FOLD_UNARY(Not, Int32, Int32, !)
        }
        else if (instr->getOpcode() >= Instruction::CastBegin && instr->getOpcode() < Instruction::CastEnd)
        {
            CastInstruction *unaryInstr = static_cast<CastInstruction *>(instr);
            Value *operand = unaryInstr->getOperand(0);
            if (!isConstant(operand))
                return;
            FOLD_UNARY(FPtoSI, Int32, Float, static_cast<int>);
            FOLD_UNARY(SItoFP, Float, Int32, static_cast<float>);
        }
        else if (instr->getOpcode() >= Instruction::CmpBegin && instr->getOpcode() < Instruction::CmpEnd)
        {
            CmpInstruction *binInstr = static_cast<CmpInstruction *>(instr);
            Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
            if (!isConstant(lhs) || !isConstant(rhs))
                return;
            FOLD_BINARY(Lt, Int32, Int32, <)
            FOLD_BINARY(Gt, Int32, Int32, >)
            FOLD_BINARY(Le, Int32, Int32, <=)
            FOLD_BINARY(Ge, Int32, Int32, >=)
            FOLD_BINARY(Eq, Int32, Int32, ==)
            FOLD_BINARY(Ne, Int32, Int32, !=)
            FOLD_BINARY(FEq, Int32, Float, ==)
            FOLD_BINARY(FNe, Int32, Float, !=)
            FOLD_BINARY(FLt, Int32, Float, <)
            FOLD_BINARY(FGt, Int32, Float, >)
            FOLD_BINARY(FLe, Int32, Float, <=)
            FOLD_BINARY(FGe, Int32, Float, >=)
        }
    }

    void ConstantFoldPass::runOnFunction(Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            for (ListNode *j = bb->getInstruction().begin(); j != bb->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                foldInstruction(instr);
            }
        }
    }
}

#undef FOLD_BINARY
#undef FOLD_UNARY