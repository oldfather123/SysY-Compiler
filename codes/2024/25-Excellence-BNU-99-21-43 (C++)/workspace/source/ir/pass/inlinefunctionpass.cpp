#include "inlinefunctionpass.h"
#include "clonebase.h"

namespace IR
{
    void InlineFunctionPass::inlineFunction(Function *parent, BasicBlock *pos, Function *callee, const std::vector<Value *> &args, CallInstruction *callInstr)
    {
        CloneBase cloneBase;
        auto temp = callee->args();
        // 拷贝参数并建立映射
        assert(temp.size() == args.size());
        for (int i = 0; i < (int)args.size(); i++)
        {
            cloneBase.setValueMapKv(temp[i], args[i]);
        }
        // 拷贝基本块并建立映射
        for (ListNode *i = callee->blocks().begin(); i != callee->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            cloneBase.addBasicBlock(BB);
        }

        // 创建结尾块
        BasicBlock *returnBlock = BasicBlock::get(callee->getIRName() + "_return");
        // 创建入口块
        BasicBlock *beginBlock = BasicBlock::get(callee->getIRName() + "_begin");
        // 创建返回值的allco
        AllocaInstruction *returnAlloca = nullptr;
        if (!(callee->getReturnType()->isVoid()))
        {
            returnAlloca = AllocaInstruction::create(callee->getReturnType());
            beginBlock->InsertInstructionBack(returnAlloca);
        }
        // 插入入口块
        parent->insertBlock(beginBlock, pos);
        pos = beginBlock;

        // 拷贝指令并建立映射
        for (ListNode *i = callee->blocks().begin(); i != callee->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            BasicBlock *newBB = static_cast<BasicBlock *>(cloneBase.getReplacedValue(BB));
            for (ListNode *j = BB->instructions.begin(); j != BB->instructions.end(); j = j->nextNode())
            {
                Instruction *instr = static_cast<Instruction *>(j);
                if (instr->getOpcode() == Instruction::Return)
                {
                    ReturnInstruction *retInst = static_cast<ReturnInstruction *>(instr);
                    if (!(retInst->isVoidReturn()))
                    {
                        auto *storeReturn = StoreInstruction::create(cloneBase.getReplacedValue(retInst->getOperand(0)), returnAlloca);
                        newBB->InsertInstructionBack(storeReturn);
                    }
                    auto newInstr = BranchInstruction::createBr(returnBlock);
                    newBB->InsertInstructionBack(newInstr);
                }
                else
                {
                    Instruction *newInstr = cloneBase.cloneInstruction(instr);
                    cloneBase.setValueMapKv(instr, newInstr);
                    newBB->InsertInstructionBack(newInstr);
                }
            }
            parent->insertBlock(newBB, pos);
            pos = newBB;
        }

        // 创建返回块
        if (!(callee->getReturnType()->isVoid()))
        {
            assert(returnAlloca != nullptr);
            auto loadReturn = LoadInstruction::create(callee->getReturnType(), returnAlloca);
            returnBlock->InsertInstructionBack(loadReturn);
            callInstr->replaceAllUsageTo(loadReturn);
        }
        parent->insertBlock(returnBlock, pos);
        pos = returnBlock;
        callInstr->waste();
        return;
    }

    void InlineFunctionPass::runOnFunction(Function *func)
    {
        entryBlockMap.clear();
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = dynamic_cast<BasicBlock *>(i);
            for (ListNode *j = BB->instructions.begin(); j != BB->instructions.end(); j = j->nextNode())
            {
                Instruction *instr = dynamic_cast<Instruction *>(j);
                if (instr->getOpcode() == Instruction::Call)
                {
                    CallInstruction *callInstr = static_cast<CallInstruction *>(instr);
                    Function *callee = callInstr->getCallee();
                    if (callee->isBuiltinFunction() || callee == func)
                        continue;
                    std::vector<Value *> args = callInstr->getArgs();
                    BasicBlock *nxt = func->splitBlock(BB, callInstr);
                    inlineFunction(func, BB, callee, args, callInstr);
                    i = nxt;
                    BB = dynamic_cast<BasicBlock *>(i);
                    j = nxt->instructions.headEmptyNode;
                }
            }
        }
    }
}
