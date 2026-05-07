#include "sub2add.h"
#include "Block.h"
#include "Instruction.h"
#include "Value.h"

void sub2add(Function* func) {
    for(auto& bb : func->getBasicBlocks()) {
        for(auto& inst : bb->instructionsRef()) {
            if(inst->insId == InsID::Binary) {
                auto binInst = dynamic_cast<BinaryInstruction*>(inst.get());
                auto rhs = binInst->getRHS();
                if(binInst->op == BinaryInstructionOps::Sub && rhs->isConst) {
                    binInst->op = BinaryInstructionOps::Add;
                    auto constRHS = dynamic_cast<Const*>(rhs);
                    // FIXME:
                    // float constVal = constRHS->getValue();
                    // auto newVal = Const::getConst(binInst->getType(), -constVal);
                    switch (constRHS->getType()->getID())
                    {
                    case TypeID::Int:
                        binInst->getOperandsRef()[1]->val = Const::getConst(binInst->getType(),-constRHS->intVal);
                        break;
                    case TypeID::Float:
                        binInst->getOperandsRef()[1]->val = Const::getConst(binInst->getType(),-constRHS->floatVal);
                        break;
                    
                    default:
                        break;
                    }
                    // FIXME:
                    // binInst->getOperandsRef()[1]->val = newVal;
                }
            }
        }
    }
}