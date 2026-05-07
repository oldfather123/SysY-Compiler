#include "AsmPassDeadBlockEliminate.h"
#include <set>
#include <queue>
#include <map>

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassDeadBlockEliminate::run(std::vector<AsmInstBasic *> insts)
    {
        std::map<AsmOperandLabel *, std::set<AsmOperandLabel *>> graph;

        AsmOperandLabel *entry = new AsmOperandLabel("function_entry", true);
        graph[entry] = std::set<AsmOperandLabel *>();

        AsmOperandLabel *nowLabel = entry;
        bool fallthrough = true;  // whether the next instruction is a fallthrough(there is no jump or return in the last label block)

        // prepare key
        for (auto inst : insts)
            if (inst->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                AsmOperandLabel *label = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();
                graph[label] = std::set<AsmOperandLabel *>();
            }
     
        // build graph
        for (auto inst : insts)
        {
            AsmInstBasic::OpcodeBasic opcode = inst->getOpcodeBasic();
            if (opcode ==AsmInstBasic::OpcodeBasic::AsmInstBlockEnd)
                continue;
            
            if (opcode == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                AsmOperandLabel *label = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();

                if (fallthrough)
                    graph[nowLabel].insert(label);
                
                nowLabel = label;
                fallthrough = true;
            }
            else if (opcode == AsmInstBasic::OpcodeBasic::AsmInstJump)
            {
                for (int i = 0; i < 3; i ++)
                {
                    AsmOperandBasic *operand = inst->getOperand(i);
                    if (operand && operand->isLabel())
                    {
                        AsmOperandLabel *label = dynamic_cast<AsmOperandLabel *>(operand);
                        graph[nowLabel].insert(label);
                    }
                }
            }
            else if (inst->willNeverReturn())
            {
                fallthrough = false;
            }
        }

        // bfs to find reachable labels
        std::set<AsmOperandLabel *> reachable;
        std::queue<AsmOperandLabel *> q;
        q.push(entry);
        reachable.insert(entry);
        while (!q.empty())
        {
            AsmOperandLabel *now = q.front();
            q.pop();
            for (auto next : graph[now])
            {
                if (reachable.find(next) == reachable.end())
                {
                    reachable.insert(next);
                    q.push(next);
                }
            }
        }

        // remove unreachable labels
        std::vector<AsmInstBasic *> newInsts;
        int instsNum = insts.size();
        for (int i = 0; i < instsNum; i++)
        {
            AsmInstBasic *inst = insts[i];
            AsmInstBasic::OpcodeBasic opcode = inst->getOpcodeBasic();
            if (opcode == AsmInstBasic::OpcodeBasic::AsmInstLabel)
            {
                AsmOperandLabel *label = dynamic_cast<AsmInstLabel *>(inst)->getAsmOperandLabel();
                if (reachable.find(label) == reachable.end())
                {
                    while (i + 1 < instsNum && insts[i + 1]->getOpcodeBasic() != AsmInstBasic::OpcodeBasic::AsmInstLabel)
                        i++;
                    continue;
                }
            }

            newInsts.push_back(inst);
        }

        return newInsts;
    }
} // namespace Backend