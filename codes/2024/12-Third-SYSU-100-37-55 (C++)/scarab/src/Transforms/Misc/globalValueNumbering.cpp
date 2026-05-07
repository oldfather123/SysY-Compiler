#include "globalValueNumbering.h"

void globalValueNumbering(FunctionPtr func) {
    vector<GetElementPtrInstruction*> candidateGEPs;

    for (auto& basicBlock : func->basicBlocks) {
        vector<InstructionPtr> newInstructions;
        for (auto& instr : basicBlock->instructions) {
            if (instr->type == GEP) {
                auto currentGEP = dynamic_cast<GetElementPtrInstruction*>(instr.get());
                bool equivalentFound = false;

                for (auto& previousGEP : candidateGEPs) {
                    if (previousGEP->from == currentGEP->from && previousGEP->index.size() == currentGEP->index.size()) {
                        bool isEquivalent = true;

                        for (int i = 0; i < previousGEP->index.size(); i++) {
                            if (previousGEP->index[i] != currentGEP->index[i] || !previousGEP->index[i]->isConst) {
                                isEquivalent = false;
                                break;
                            }
                        }

                        if (isEquivalent) {
                            auto previousBB = previousGEP->basicblock;
                            auto currentBB = currentGEP->basicblock;

                            if (currentBB->allDominators.count(previousBB)) {
                                equivalentFound = true;
                                replaceVarByVar(currentGEP->reg, previousGEP->reg);
                                deleteUser(currentGEP->reg);
                                break;
                            }
                        }
                    }
                }

                if (!equivalentFound) {
                    candidateGEPs.push_back(currentGEP);
                    newInstructions.push_back(instr);
                }
            }
            else {
                newInstructions.push_back(instr);
            }
        }
        basicBlock->instructions = std::move(newInstructions);
    }
}