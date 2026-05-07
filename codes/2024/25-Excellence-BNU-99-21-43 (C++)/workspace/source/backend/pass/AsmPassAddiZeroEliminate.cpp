#include "AsmPassAddiZeroEliminate.h"

#include <iostream>

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassAddiZeroEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;
        for (auto inst : insts)
        {
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstAdd)
            {
                AsmInstAdd *addInst = static_cast<AsmInstAdd *>(inst);
                if ((addInst->getOpcode() == AsmInstAdd::Opcode::ADDI || addInst->getOpcode() == AsmInstAdd::Opcode::ADDIW)
                    && addInst->getOperand(0) == addInst->getOperand(1)
                    && addInst->getOperand(2)->isImmediate()
                    && static_cast<AsmOperandImmediate *>(addInst->getOperand(2))->getValue() == 0
                )
                {
                    continue;
                }
            }
            newInsts.push_back(inst);
        }
        return newInsts;
    }
}