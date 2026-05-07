#include "constStayRight.h"
#include "Block.h"
#include "Instruction.h"
#include "Value.h"

// contain bug
void constStayRight(Function* func) {
    for(auto& bb : func->getBasicBlocks()) {
        for(auto& inst : bb->instructionsRef()) {
            if(inst->insId == InsID::Binary) {
                auto binInst = dynamic_cast<BinaryInstruction*>(inst.get());
                if(binInst->op == BinaryInstructionOps::Div || binInst->op == BinaryInstructionOps::Sub ||
                   binInst->op == BinaryInstructionOps::Shl || binInst->op == BinaryInstructionOps::AShr ||
                   binInst->op == BinaryInstructionOps::FDiv || binInst->op == BinaryInstructionOps::FSub) {
                    continue;
                }
                if(binInst->getLHS()->isConst) {
                    binInst->getOperandsRef()[0].swap(binInst->getOperandsRef()[1]);
                }
            }
        }
    }
}