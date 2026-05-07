#include "AsmPassRedundantUnconditionalJumpEliminate.h"

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassRedundantUnconditionalJumpEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;

        int numInsts = insts.size();
        for (int i = 0; i < numInsts; i++)
        {
            if (insts[i]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstJump)
            {
                AsmInstJump *jumpInst = dynamic_cast<AsmInstJump *>(insts[i]);
                if (jumpInst->getOpcode() == AsmInstJump::Opcode::J)
                {
                    if (i + 1 < numInsts && insts[i + 1]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
                    {
                        AsmOperandLabel *jumpLabel = jumpInst->getAsmOperandLabel();
                        AsmOperandLabel *nextLabel = dynamic_cast<AsmInstLabel *>(insts[i + 1])->getAsmOperandLabel();
                        if (*jumpLabel == *nextLabel)
                        {
                            continue;
                        }
                    }
                    else if (i + 2 < numInsts
                             && insts[i + 1]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstBlockEnd
                             && insts[i + 2]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
                    {
                        AsmOperandLabel *jumpLabel = jumpInst->getAsmOperandLabel();
                        AsmOperandLabel *nextLabel = dynamic_cast<AsmInstLabel *>(insts[i + 2])->getAsmOperandLabel();
                        if (*jumpLabel == *nextLabel)
                        {
                            continue;
                        }
                    }
                }
            }
            newInsts.push_back(insts[i]);
        }

        return newInsts;
    }
}