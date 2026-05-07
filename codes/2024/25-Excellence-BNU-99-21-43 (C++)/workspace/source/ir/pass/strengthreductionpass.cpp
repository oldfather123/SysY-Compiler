#include "strengthreductionpass.h"

namespace IR
{
    bool checkInstructionOpcode(Value *instr, const unsigned int type)
    {
        return instr->isInstruction() && static_cast<Instruction *>(instr)->getOpcode() == type;
    }

    bool StrengthReductionPass::mulConstantTrans(BasicBlock *bb)
    {
        bool changed = false;
        for (ListNode *i = bb->getInstruction().begin(); i != bb->getInstruction().end(); i = i->nextNode())
        {
            Instruction *instr = static_cast<Instruction *>(i);
            if (instr->getOpcode() == Instruction::Mul)
            {
                BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                auto lhs = binInstr->getOperand(0);
                auto rhs = binInstr->getOperand(1);
                if (rhs->isInstruction())
                {
                    std::swap(lhs, rhs);
                }
                if (rhs->isConstantInt32() && lhs->isInstruction() &&
                    static_cast<Instruction *>(lhs)->getOpcode() == Instruction::Mul)
                {
                    auto lhsInstr = static_cast<BinaryInstruction *>(lhs);
                    int val = static_cast<ConstantInt32 *>(rhs)->getValue();
                    if (lhsInstr->getOpcode() == Instruction::Mul)
                    {
                        auto lhsLhs = lhsInstr->getOperand(0);
                        auto lhsRhs = lhsInstr->getOperand(1);
                        if (lhsRhs->isInstruction())
                            std::swap(lhsLhs, lhsRhs);
                        if (lhsRhs->isConstantInt32())
                        {
                            int lhsRhsVal = static_cast<ConstantInt32 *>(lhsRhs)->getValue();
                            int totVal = lhsRhsVal * val;
                            // 更换为新的常数
                            auto newConstantInt32 = ConstantInt32::get(totVal);
                            BinaryInstruction *newInstr = BinaryInstruction::createMul(BasicType::getInt32Type(), newConstantInt32, lhsLhs);
                            // 更新新的指令
                            bb->InsertInstruction(newInstr, binInstr);
                            binInstr->replaceAllUsageTo(newInstr);
                            binInstr->waste();
                            changed = true;
                        }
                    }
                }
            }
        }
        return changed;
    }

    // 处理Add指令，形如 c = b + a, b = a * 2 => c = a * 3
    bool StrengthReductionPass::add2Mul(BasicBlock *bb)
    {
        bool changed = false;
        for (ListNode *i = bb->getInstruction().begin(); i != bb->getInstruction().end(); i = i->nextNode())
        {
            Instruction *instr = static_cast<Instruction *>(i);

            if (instr->getOpcode() == Instruction::Add)
            {
                BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                auto lhs = binInstr->getOperand(0);
                auto rhs = binInstr->getOperand(1);
                if (lhs == rhs)
                {
                    BinaryInstruction *newInstr = BinaryInstruction::createMul(BasicType::getInt32Type(), lhs, ConstantInt32::get(2));
                    bb->InsertInstruction(newInstr, instr);
                    instr->replaceAllUsageTo(newInstr);
                    instr->waste();
                    changed = true;
                    continue;
                }

                if (checkInstructionOpcode(lhs, Instruction::Mul))
                {
                    auto mulInstr = static_cast<BinaryInstruction *>(lhs);
                    Value *lop = mulInstr->getOperand(0);
                    Value *rop = mulInstr->getOperand(1);
                    if (lop->isConstantInt32())
                    {
                        std::swap(lop, rop);
                    }
                    if (lop == rhs && rop->isConstantInt32())
                    {
                        ConstantInt32 *newConstInt = ConstantInt32::get(1 + static_cast<ConstantInt32 *>(rop)->getValue());
                        auto newInstr = BinaryInstruction::createMul(BasicType::getInt32Type(), newConstInt, rhs);
                        bb->InsertInstruction(newInstr, instr);
                        instr->replaceAllUsageTo(newInstr);
                        instr->waste();
                        changed = true;
                        continue;
                    }
                }
                if (checkInstructionOpcode(rhs, Instruction::Mul))
                {
                    auto mulInstr = static_cast<BinaryInstruction *>(rhs);
                    Value *lop = mulInstr->getOperand(0);
                    Value *rop = mulInstr->getOperand(1);
                    if (lop->isConstantInt32())
                    {
                        std::swap(lop, rop);
                    }
                    if (lop == lhs && rop->isConstantInt32())
                    {
                        ConstantInt32 *newConstInt = ConstantInt32::get(1 + static_cast<ConstantInt32 *>(rop)->getValue());
                        auto newInstr = BinaryInstruction::createMul(BasicType::getInt32Type(), newConstInt, lhs);
                        bb->InsertInstruction(newInstr, instr);
                        instr->replaceAllUsageTo(newInstr);
                        instr->waste();
                        changed = true;
                        continue;
                    }
                }
            }
        }

        return changed;
    }

    // 处理Add指令，形如 a = a + 1, a = a + 1 -> a = a + 2
    bool StrengthReductionPass::addConstantTrans(BasicBlock *bb)
    {
        bool changed = false;
        for (ListNode *i = bb->getInstruction().begin(); i != bb->getInstruction().end(); i = i->nextNode())
        {
            Instruction *instr = static_cast<Instruction *>(i);

            // 处理Add指令，形如 c = b + 1, b = a + 1 -> c = a + 2
            if (instr->getOpcode() == Instruction::Add)
            {
                BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                auto lhs = binInstr->getOperand(0);
                auto rhs = binInstr->getOperand(1);
                int lId = 0, rId = 1;
                if (rhs->isInstruction())
                {
                    std::swap(lhs, rhs);
                    std::swap(lId, rId);
                }
                if (rhs->isConstantInt32() && lhs->isInstruction() &&
                    static_cast<Instruction *>(lhs)->getOpcode() == Instruction::Add)
                {
                    auto lhsInstr = static_cast<BinaryInstruction *>(lhs);
                    int val = static_cast<ConstantInt32 *>(rhs)->getValue();
                    if (lhsInstr->getOpcode() == Instruction::Add)
                    {
                        auto lhsLhs = lhsInstr->getOperand(0);
                        auto lhsRhs = lhsInstr->getOperand(1);
                        if (lhsRhs->isInstruction())
                            std::swap(lhsLhs, lhsRhs);
                        if (lhsRhs->isConstantInt32())
                        {
                            int lhsRhsVal = static_cast<ConstantInt32 *>(lhsRhs)->getValue();
                            int totVal = lhsRhsVal + val;
                            // 更换为新的常数
                            auto newConstantInt32 = ConstantInt32::get(totVal);
                            BinaryInstruction *newInstr = BinaryInstruction::createAdd(BasicType::getInt32Type(), newConstantInt32, lhsLhs);
                            // 更新新的指令
                            bb->InsertInstruction(newInstr, binInstr);
                            binInstr->replaceAllUsageTo(newInstr);
                            binInstr->waste();
                            changed = true;
                        }
                    }
                }
            }
        }
        return changed;
    }

    void StrengthReductionPass::runOnBlock(BasicBlock *bb)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;
            changed |= addConstantTrans(bb);
            changed |= add2Mul(bb);
            changed |= mulConstantTrans(bb);
        }
    }

    void StrengthReductionPass::runOnFunction(IR::Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            runOnBlock(bb);
        }
    }
}