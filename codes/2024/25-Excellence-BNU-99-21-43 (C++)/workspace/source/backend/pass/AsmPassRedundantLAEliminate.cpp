#include "AsmPassRedundantLAEliminate.h"
#include <iostream>

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassRedundantLAEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;
        records.clear();

        int numInsts = insts.size();
        int counter = 0;
        for (int i = 0; i < numInsts; i++)
        {
            auto inst = insts[i];
            // std::cerr << "inst: " << inst->toString() << std::endl;
            
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstBlockEnd)
                continue;

            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
                records.clear();
            else if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLoad
                && dynamic_cast<AsmInstLoad *>(inst)->getOpcode() == AsmInstLoad::Opcode::LA
            )
            {
                AsmInstLoad *loadInst = dynamic_cast<AsmInstLoad *>(inst);
                Record tmpRecord = {
                    dynamic_cast<AsmOperandRegister*>(loadInst->getOperand(0)),
                    dynamic_cast<AsmOperandLabel*>(loadInst->getOperand(1))
                };

                auto p = records.find(tmpRecord);
                if (p != records.end())
                {

                    // std::cerr << "redundant LA: , \n" << std::endl;
                    // std::cerr << "  record : " << p->first->toString() << ", " << p->second->toString() << std::endl;
                    // std::cerr << "  tmpRecord : " << tmpRecord.first->toString() << ", " << tmpRecord.second->toString() << std::endl;
                    counter ++;
                    continue;
                }
                else
                {
                    std::vector<Record> toRemove;
                    for (const auto& record : records)
                    {
                        if (*record.first == *tmpRecord.first)
                        {
                            toRemove.push_back(record);
                        }
                    }

                    for (const auto& record : toRemove)
                    {
                        records.erase(record);
                    }

                    records.insert(tmpRecord);
                }
            }
            else
            {
                std::set<AsmOperandRegister*> def = inst->getDef();

                std::vector<Record> toRemove;
                for (const auto& record : records)
                {
                    if (def.find(record.first) != def.end())
                    {
                        toRemove.push_back(record);
                    }
                }

                for (const auto& record : toRemove)
                {
                    records.erase(record);
                }
            }

            newInsts.push_back(inst);
        }

        // std::cerr << "redundant LA: " << counter << std::endl;

        return newInsts;
    }
}
