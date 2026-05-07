#include "commonsubexpressioneliminationpass.h"

namespace IR
{
    void CommonSubExpressionEliminationPass::runOnFunction(Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            bool changed = true;
            while (changed)
                changed = runOnBlock(BB);
        }
    }

    POO CommonSubExpressionEliminationPass::createUniqueKey(Instruction *instr)
    {
        std::vector<Value *> operands = instr->getOperands();
        unsigned int opcode = instr->getOpcode();
        switch (opcode)
        {
        case Instruction::Add:
        case Instruction::Mul:
        case Instruction::FAdd:
        case Instruction::FMul:
        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor:
        case Instruction::FNe:
        case Instruction::FEq:
        case Instruction::Ne:
        case Instruction::Eq:
            std::sort(operands.begin(), operands.end());
        default:
            std::vector<variableKey> keys;
            keys.push_back(instr->getType());
            for (auto operand : operands)
            {
                if (operand->isConstantInt32())
                    keys.push_back(static_cast<ConstantInt32 *>(operand)->getValue());
                else if (operand->isConstantFloat())
                    keys.push_back(static_cast<ConstantFloat *>(operand)->getValue());
                else
                    keys.push_back(operand);
            }
            return std::make_pair(opcode, keys);
        }
    }

    bool CommonSubExpressionEliminationPass::checkAfterLastDef(Instruction *instr)
    {
        auto it = instrIndex.find(instr);
        assert(it != instrIndex.end());
        int index = it->second;
        for (auto operand : instr->getOperands())
        {
            auto it = lastDef.find(operand);
            if (it == lastDef.end())
            {
                continue;
            }
            if (instrIndex[it->second] > index)
            {
                return false;
            }
        }
        return true;
    }

    void CommonSubExpressionEliminationPass::updateLastDef(Instruction *instr)
    {
        // 更新最后一次定义
        // 首先如果是Store指令，则对于其 dst 操作数，如果是一个alloca或者全局变量，则更新 lastDef[dst] = instr
        // 如果是一个参数且是指针，则与上面一样
        // 如果是一个gep，则更新gep的base指针，即lastDef[gep->base] = instr
        if (instr->getOpcode() == Instruction::Store)
        {
            auto dst = instr->getOperand(1);
            if (dst->isArgument() || dst->isGlobalVariable())
                lastDef[dst] = instr;
            else if (dst->isInstruction() && static_cast<Instruction *>(dst)->getOpcode() == Instruction::Alloca)
                lastDef[dst] = instr;
            else if (dst->isInstruction() && static_cast<Instruction *>(dst)->getOpcode() == Instruction::GEP)
            {
                auto gep = static_cast<GetElementPtrInstruction *>(dst);
                lastDef[gep->getOperand(0)] = instr;
            }
        }
        else if (instr->getOpcode() == Instruction::Load) // 如果是一个load，则更新lastDef[instr] = lastDef[instr->src]
        {
            auto res = instr->getOperand(0);
            if (res->isInstruction() && static_cast<Instruction *>(res)->getOpcode() == Instruction::GEP)
                res = static_cast<GetElementPtrInstruction *>(res)->getOperand(0);
            if (lastDef.find(res) != lastDef.end())
                lastDef[instr] = lastDef[res];
        }
        else if (instr->getOpcode() == Instruction::Call) // 如果是一个call，则清空所有的lastDef和commonSubExpressions
        {
            lastDef.clear();
            commonSubExpressions.clear();
        }
        else // 如果是一个普通运算，则lastDef[instr] = instr
            lastDef[instr] = instr;
    }

    bool CommonSubExpressionEliminationPass::runOnBlock(BasicBlock *bb)
    {
        bool changed = false;
        commonSubExpressions.clear();
        lastDef.clear();
        instrIndex.clear();
        int index = 1;
        for (ListNode *i = bb->getInstruction().begin(); i != bb->getInstruction().end(); i = i->nextNode())
        {
            Instruction *instr = static_cast<Instruction *>(i);
            POO key = createUniqueKey(instr);
            // 首先判断是否存在相同表达式
            if (instr->getOpcode() != Instruction::Alloca &&
                instr->getOpcode() != Instruction::Return &&
                instr->getOpcode() != Instruction::Call &&
                instr->getOpcode() != Instruction::Phi &&
                instr->getOpcode() != Instruction::BR &&
                instr->getOpcode() != Instruction::Store &&
                instr->getOpcode() != Instruction::InderectBr &&
                commonSubExpressions.find(key) != commonSubExpressions.end())
            {
                // 如果存在相同表达式, 则还需要判断是否在最后一次定义之后
                Instruction *oldInstr = commonSubExpressions[key];

                if (checkAfterLastDef(oldInstr))
                {
                    instr->replaceAllUsageTo(oldInstr);
                    bb->getInstruction().remove(instr);
                    changed = true;
                    continue;
                }
            }
            // 如果不存在相同表达式，更新最后一次定义
            updateLastDef(instr);
            instrIndex[instr] = index++;
            commonSubExpressions[key] = instr;
        }
        return changed;
    }

}