#include "mem2reg.h"
#include "domtree.h"
#include "dataflowanalysis.h"

namespace IR
{
    void Mem2RegPass::removeUselessStoreLoad(Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            for (ListNode *j = BB->getInstruction().begin(); j != BB->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                if (instr->getOpcode() == Instruction::Load)
                {
                    auto load = static_cast<LoadInstruction *>(instr);
                    auto src = load->getSrc();
                    if (src->isInstruction() &&
                        isNeedAlloca(static_cast<Instruction *>(src)))
                    {
                        instr->waste();
                    }
                }
                else if (instr->getOpcode() == Instruction::Store)
                {
                    auto store = static_cast<StoreInstruction *>(instr);
                    auto dst = store->getDest();
                    if (dst->isInstruction() &&
                        isNeedAlloca(static_cast<Instruction *>(dst)))
                    {
                        instr->waste();
                    }
                }
            }
        }
    }

    void Mem2RegPass::removeUseLessAlloca(Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            for (ListNode *j = BB->getInstruction().begin(); j != BB->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                if (isNeedAlloca(instr))
                    instr->waste();
            }
        }
    }

    bool Mem2RegPass::isNeedAlloca(Instruction *instr)
    {
        if (instr->getOpcode() == Instruction::Alloca)
        {
            static std::map<Instruction *, bool> cache;
            if (cache.find(instr) != cache.end())
                return cache[instr];

            for (ListNode *i = instr->useList.begin(); i != instr->useList.end(); i = i->nextNode())
            {
                auto user = static_cast<Use *>(i)->user;
                if (user->isInstruction())
                {
                    auto inst = static_cast<Instruction *>(user);
                    if (!(inst->getOpcode() == Instruction::Load ||
                          inst->getOpcode() == Instruction::Store))
                    {
                        cache[instr] = false;
                        return false;
                    }
                }
                else
                {
                    cache[instr] = false;
                    return false;
                }
            }
            cache[instr] = true;
            return true;
        }
        return false;
    }

    Value *Mem2RegPass::getAllocaLastDef(Function *fun, BasicBlock *bb, AllocaInstruction *alloca)
    {
        if (lastDef.find(bb) == lastDef.end())
        {
            lastDef[bb] = std::map<AllocaInstruction *, Value *>();
        }
        auto &BBLastDef = lastDef[bb];
        if (BBLastDef.find(alloca) == BBLastDef.end())
        {
            if (bb == fun->getEntryBlock())
            {
                BBLastDef[alloca] = Constant::getZeroValueForType(alloca->getAllocaType());
            }
            else
            {
                auto instr = PhiInstruction::create(alloca->getAllocaType(), {}, "phi");
                bb->InsertInstructionFront(instr);
                auto temp = bb->getInstruction().begin();
                assert(temp != bb->getInstruction().end());
                BBLastDef[alloca] = static_cast<Instruction *>(temp);
                worklist.push(std::make_pair(alloca, instr));
            }
        }
        return BBLastDef[alloca];
    }

    void Mem2RegPass::runOnFunction(Function *func)
    {
        if (func->isBuiltinFunction())
            return;
        lastDef.clear();
        worklist = std::queue<PAP>();
        // 插入 phi 节点
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *bb = static_cast<BasicBlock *>(i);
            for (ListNode *j = bb->getInstruction().begin(); j != bb->getInstruction().end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                if (instr->getOpcode() == Instruction::Load)
                {
                    auto load = static_cast<LoadInstruction *>(instr);
                    auto src = load->getSrc();
                    if (src->isInstruction() &&
                        isNeedAlloca(static_cast<Instruction *>(src)))
                    {
                        auto alloca = static_cast<AllocaInstruction *>(src);
                        load->replaceAllUsageTo(getAllocaLastDef(func, bb, alloca));
                    }
                }
                else if (instr->isInstruction() &&
                         static_cast<Instruction *>(instr)->getOpcode() == Instruction::Store)
                {
                    auto store = static_cast<StoreInstruction *>(instr);
                    auto dst = store->getDest();
                    if (dst->isInstruction() &&
                        isNeedAlloca(static_cast<Instruction *>(dst)))
                    {
                        auto user = dynamic_cast<Value *>(store->getSrc());
                        assert(user != nullptr);
                        lastDef[bb][static_cast<AllocaInstruction *>(dst)] = user;
                    }
                }
            }
        }
        //  更新 phi 节点

        while (!worklist.empty())
        {
            auto [alloca, phi] = worklist.front();
            worklist.pop();
            BasicBlock *bb = phi->getParentBB();
            assert(bb != nullptr);
            for (auto &prevBB : bb->getPredBlock())
            {
                auto lastDef = getAllocaLastDef(func, prevBB, alloca);
                phi->insertIncomingValue(std::make_pair(lastDef, prevBB));
            }
        }

        removeUselessStoreLoad(func);

        removeUseLessAlloca(func);
    }
}