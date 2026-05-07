#include "finalpass.h"

namespace IR
{
    void FinalPass::runOnFunction(IR::Function *func)
    {
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            if (BB->nextNode() != func->blocks().end())
            {
                // BB->getInstruction().empty() && 
                bool hasBranch = false;
                for (ListNode *j = BB->getInstruction().begin(); j != BB->getInstruction().end(); j = j->nextNode())
                {
                    Instruction *inst = static_cast<Instruction *>(j);
                    if (inst->getOpcode() == Instruction::BR || inst->getOpcode() == Instruction::Return)
                    {
                        hasBranch = true;
                        break;
                    }
                }

                if (!hasBranch)
                {
                    BasicBlock *nxtBB = static_cast<BasicBlock *>(BB->nextNode());
                    BranchInstruction *branch = BranchInstruction::createBr(nxtBB);
                    BB->InsertInstructionBack(branch);
                }
            }
        }
    }
}