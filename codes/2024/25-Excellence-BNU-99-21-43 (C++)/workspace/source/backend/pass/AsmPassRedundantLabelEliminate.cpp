#include "AsmPassRedundantLabelEliminate.h"
#include <set>

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassRedundantLabelEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        std::set<AsmOperandLabel *, AsmOperandLabel::AsmOperandLabelCmp> labelSet;

        for (auto inst : insts)
            for (int i = 0; i < 3; i ++)
                if (inst->getOperand(i) != nullptr && inst->getOperand(i)->isLabel())
                    labelSet.insert(dynamic_cast<AsmOperandLabel *>(inst->getOperand(i)));
        
        std::vector<AsmInstBasic *> newInsts;
        for (auto inst : insts)
        {
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                AsmOperandLabel *label = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();
                if (labelSet.find(label) == labelSet.end())
                    continue;
            }
            newInsts.push_back(inst);
        }

        return newInsts;
    }
}