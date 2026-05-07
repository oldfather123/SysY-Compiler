#include "simplifycodepass.h"
#include "error.h"
#include <queue>

namespace IR
{
    // 死代码删除的原理：遍历每个函数的基本块，如果一个语句没有被别人引用，那么就可以删除该语句
    // 但是需要注意，有些语句即使没有使用，也不能删除，下面列举不能删除的Instruction：
    // StoreInst, CallInst, TerminatorInst,
    bool SimplifyCodePass::deadCodeDelete(IR::Function *func)
    {
        bool check = false;
        for (ListNode *i = func->blocks().rbegin(); i != func->blocks().rend(); i = i->prevNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            for (ListNode *j = bb->getInstruction().rbegin(); j != bb->getInstruction().rend(); j = j->prevNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                // 如果该指令没有被引用，那么就可以删除
                if (instr->isUseless())
                {
                    instr->waste();
                    check = true;
                }
            }
        }
        return check;
    }

    bool SimplifyCodePass::simplifyPhi(IR::Function *func)
    {
        bool allChanged = false;
        bool changed = false;
        do
        {
            changed = false;
            for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
            {
                BasicBlock *bb = static_cast<BasicBlock *>(i);
                for (ListNode *j = bb->getInstruction().begin(); j != bb->getInstruction().end(); j = j->nextNode())
                {
                    Instruction *instr = static_cast<Instruction *>(j);
                    if (instr->getOpcode() == Instruction::Phi)
                    {
                        PhiInstruction *phi = static_cast<PhiInstruction *>(instr);
                        auto value = phi->getIncomingValue();
                        // 如果一个Phi指令仅有一个操作数，那么可以简化
                        if (value.size() == 1)
                        {
                            phi->replaceAllUsageTo(value[0].first);
                            phi->waste();
                            changed = true;
                            allChanged = true;
                            continue;
                        }
                        bool changed = true;
                        while (changed)
                        {
                            changed = false;
                            for (int i = 0; i < phi->getValueBBSize(); i++)
                            {
                                auto [value, BB] = phi->getValueBB(i);
                                auto pred = bb->getPredBlock();
                                // std::cerr << bb->getIRName() << " pred size: " << pred.size() << std::endl;
                                // std::cerr << '\t' << BB->getIRName() << ' ' << value->getIRName() << std::endl;
                                if (pred.find(BB) == pred.end())
                                {
                                    phi->removeUseFromVector(phi->uses[2 * i + 1]);
                                    changed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } while (changed);
        return allChanged;
    }

    bool SimplifyCodePass::simplifyCondBranch(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            ListNode *j = bb->getInstruction().rbegin();
            if (j == bb->getInstruction().rend())
                continue;
            Instruction *instr = static_cast<Instruction *>(j);
            if (instr->getOpcode() == Instruction::BR)
            {
                BranchInstruction *br = static_cast<BranchInstruction *>(instr);
                if (br->isConditional())
                {
                    Value *cond = br->getCondition();
                    auto trueBB = br->getTrueBlock();
                    auto falseBB = br->getFalseBlock();
                    if (cond->isConstantInt32())
                    {
                        if (static_cast<ConstantInt32 *>(cond)->getValue() == 0)
                        {
                            br->waste();
                            changed = true;
                            bb->InsertInstructionBack(BranchInstruction::createBr(falseBB));
                        }
                        else
                        {
                            br->waste();
                            changed = true;
                            bb->InsertInstructionBack(BranchInstruction::createBr(trueBB));
                        }
                    }
                }
            }
        }
        return changed;
    }

    bool SimplifyCodePass::simplifyDirectBranch(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            auto bb = static_cast<BasicBlock *>(i);
            if (bb->isDirectBrBlock())
            {
                auto br = static_cast<BranchInstruction *>(bb->getInstruction().back());
                if (bb->nextNode() != func->blocks().end() && br->getUnconditionalBlock() == bb->nextNode())
                {
                    br->waste();
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool SimplifyCodePass::simplifyIdentity(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            for (ListNode *j = bb->getInstruction().begin(); j != bb->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);

                // a = a + 0
                if (instr->getOpcode() == Instruction::Add)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (lhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(lhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(rhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                    else if (rhs->isConstantInt32() &&
                             static_cast<ConstantInt32 *>(rhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(lhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = a - 0
                if (instr->getOpcode() == Instruction::Sub)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (rhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(rhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(lhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = a * 1
                if (instr->getOpcode() == Instruction::Mul)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (lhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(lhs)->getValue() == 1)
                    {
                        binInstr->replaceAllUsageTo(rhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                    else if (rhs->isConstantInt32() &&
                             static_cast<ConstantInt32 *>(rhs)->getValue() == 1)
                    {
                        binInstr->replaceAllUsageTo(lhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = a * 0
                if (instr->getOpcode() == Instruction::Mul)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (lhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(lhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(ConstantInt32::get(0));
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                    else if (rhs->isConstantInt32() &&
                             static_cast<ConstantInt32 *>(rhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(ConstantInt32::get(0));
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = a / 1
                if (instr->getOpcode() == Instruction::Div)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (rhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(rhs)->getValue() == 1)
                    {
                        binInstr->replaceAllUsageTo(lhs);
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = a % 1
                if (instr->getOpcode() == Instruction::Rem)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *rhs = binInstr->getOperand(1);
                    if (rhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(rhs)->getValue() == 1)
                    {
                        binInstr->replaceAllUsageTo(ConstantInt32::get(0));
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }

                // a = 0 / a
                if (instr->getOpcode() == Instruction::Div)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0);
                    if (lhs->isConstantInt32() &&
                        static_cast<ConstantInt32 *>(lhs)->getValue() == 0)
                    {
                        binInstr->replaceAllUsageTo(ConstantInt32::get(0));
                        binInstr->waste();
                        changed = true;
                        continue;
                    }
                }
            }
        }
        return changed;
    }
    // b = a + a; => b = a * 2;
    bool SimplifyCodePass::simplifyMultiAddToMul(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            for (ListNode *j = bb->getInstruction().begin(); j != bb->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                if (instr->getOpcode() == Instruction::Add)
                {
                    BinaryInstruction *binInstr = static_cast<BinaryInstruction *>(instr);
                    Value *lhs = binInstr->getOperand(0), *rhs = binInstr->getOperand(1);
                    if (lhs->isConstantInt32() && rhs->isConstantInt32())
                    {
                        int l = static_cast<ConstantInt32 *>(lhs)->getValue();
                        int r = static_cast<ConstantInt32 *>(rhs)->getValue();
                        binInstr->replaceAllUsageTo(ConstantInt32::get(l + r));
                        binInstr->waste();
                        changed = true;
                    }
                }
            }
        }
        return changed;
    }

    bool SimplifyCodePass::deleteEmptyBlock(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            if (bb->getInstruction().empty())
            {
                bb->waste();
                changed = true;
            }
        }
        return changed;
    }

    bool SimplifyCodePass::mergeBlock(IR::Function *func)
    {
        bool changed = false;
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            auto next = bb->nextNode();
            while (next != func->blocks().end())
            {
                BasicBlock *nxt = static_cast<BasicBlock *>(next);
                if (BasicBlock::checkNeignbour(bb, nxt))
                {
                    func->mergeBlock(bb, nxt);
                    changed = true;
                }
                else
                    break;
                next = bb->nextNode();
            }
        }
        return changed;
    }

    bool SimplifyCodePass::unreachableBlockDelete(IR::Function *func)
    {
        std::map<BasicBlock *, bool> vis;
        std::vector<BasicBlock *> worklist = {func->getEntryBlock()};
        vis[func->getEntryBlock()] = true;
        // BFS 遍历所有可达块
        for (int i = 0, j = 1; i < j; i++)
        {
            BasicBlock *bb = worklist[i];
            for (auto &succ : bb->getSuccBlock())
            {
                if (!vis[succ])
                {
                    vis[succ] = true;
                    j++;
                    worklist.push_back(succ);
                }
            }
        }

        bool check = false;
        // 如果一个块不可达，那么就可以删除
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            if (!vis[bb])
            {
                if (bb == func->getEntryBlock())
                {
                    Error::Error("SimplifyCodePass::unreachableBlockDelete", "Entry block is unreachable");
                    continue;
                }
                bb->waste();
                check = true;
            }
        }
        return check;
    }

    void SimplifyCodePass::runOnFunction(IR::Function *func)
    {
        if (func->isBuiltinFunction())
            return;
        bool changed = true;
        while (changed)
        {
            changed = false;
            changed |= deadCodeDelete(func);
            changed |= unreachableBlockDelete(func);
            changed |= mergeBlock(func);
            changed |= simplifyIdentity(func);
            changed |= simplifyMultiAddToMul(func);
            changed |= simplifyPhi(func);
            changed |= simplifyCondBranch(func);
            changed |= simplifyDirectBranch(func);
        }
    }
}