#include "riscv.h"

void replaceBlockDFS(MProg* mProg) {
    for(auto mFunc : mProg->mFuncs) {
        Array<MBlock*> newOrder;
        Array<MBlock*> stack;
        stack.push(mFunc->mBlocks[0]);
        while(!stack.empty()) {
            auto curBB = stack.back();
            stack.pop();
            if(newOrder.find(curBB) != -1)
                continue;
            newOrder.push(curBB);
            if(curBB->terminator->tag == MI_RETURN)
                continue;
            assert(curBB->terminator->tag == MI_BRANCH);
            auto brMI = (MI_Branch*)curBB->terminator;
            if(brMI->cond == BR_JU) {
                stack.push(brMI->TBlock);
            } else {
                if(brMI->TBlock->loopDepth <= curBB->loopDepth) {
                    stack.push(brMI->TBlock);
                    stack.push(brMI->FBlock);
                } else {
                    stack.push(brMI->FBlock);
                    stack.push(brMI->TBlock);
                }
            }
        }
        mFunc->mBlocks = newOrder;
        for(int i = 0; i < mFunc->mBlocks.size(); i++) {
            mFunc->mBlocks[i]->id = i;
        }
    }
}
