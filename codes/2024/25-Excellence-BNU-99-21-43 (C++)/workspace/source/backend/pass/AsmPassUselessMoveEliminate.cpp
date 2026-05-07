#include "AsmPassUselessMoveEliminate.h"

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassUselessMoveEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        insts = _removeMoveA2A(insts);
        insts = _removeMoveA2BB2A(insts);

        return insts;
    }

    std::vector<AsmInstBasic *> AsmPassUselessMoveEliminate::_removeMoveA2A(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;
        for (auto inst : insts)
        {
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstMove 
                && inst->getOperand(0) == inst->getOperand(1))
            {
                continue;
            }
            newInsts.push_back(inst);
        }
        return newInsts;
    }

    std::vector<AsmInstBasic *> AsmPassUselessMoveEliminate::_removeMoveA2BB2A(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;
        int numInsts = insts.size();
        for (int i = 0; i < numInsts; i++)
        {
            if (insts[i]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstMove 
                && i + 1 < numInsts
                && insts[i + 1]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstMove
                && insts[i]->getOperand(0) == insts[i + 1]->getOperand(1)
                && insts[i]->getOperand(1) == insts[i + 1]->getOperand(0))
            {
                newInsts.push_back(insts[i]);
                i++;
                continue;
            }
            newInsts.push_back(insts[i]);
        }
        return newInsts;
    }
}