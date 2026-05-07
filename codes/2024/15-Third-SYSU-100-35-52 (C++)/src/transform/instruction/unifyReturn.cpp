#include "IRbuilder.h"
#include "Instruction.h"
#include "Value.h"
#include "unifyReturn.h"

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/ControlFlow/UnifyReturn.cpp#L4

bool runUnifyReturn(FunctionPtr func) {
    if(func->noReturn)
        return false;
    if(func->basicBlocks.size() == 1)
        return false;
    std::vector<Instruction*> rets;
    for(auto& block : func->basicBlocks) {
        if(block->getTerminator()->insId == InsID::Return) {
            rets.push_back(block->getTerminator());
        }
    }
    if(rets.size() <= 1)
        return false;
    IRbuilder builder{};
    builder.setCurFunc(func);
    const auto exitBlock = builder.addBasicBlock("exit");
    builder.setCurBasicBlock(exitBlock);
    PhiInstruction* retValue = nullptr;
    if(auto retType = func->getType(); !retType->isVoid()) {
        retValue = new PhiInstruction(retType);  // NOLINT
        insertBefore(exitBlock, exitBlock->instructionsRef().begin(), retValue);
    }

    for(int i = 0; i < rets.size(); i++) {
        auto ret = rets[i];
        const auto block = ret->getBasicBlock();
        if(retValue) {
            retValue->addIncoming(block, ret->getOperand(0));
        }
        auto& instructions = block->instructionsRef();
        instructions.pop_back();
        //const auto branch = make<BranchInst>(exitBlock);
        auto branch = new BrInstruction(exitBlock);  // NOLINT
        insertBefore(block, block->instructionsRef().end(),branch);
    }

    if(retValue) {
        //builder.makeOp<ReturnInst>(retValue);
        builder.createRet(retValue);
    } else {
        //builder.makeOp<ReturnInst>();
        builder.createRet(Void::get());
    }

    return true;
}