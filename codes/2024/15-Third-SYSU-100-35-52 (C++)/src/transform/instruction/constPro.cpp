#include "constPro.h"

void constPropagate(Module& mod) {
    for (auto& func : mod.getGlobalFunctions()) {
        for (auto& bb : func->getBasicBlocks()) {
            for (auto& inst : bb->instructionsRef()) {
                // if (inst->getInsID() == InsID::Load) {
                //     auto loadInst = dynamic_cast<LoadInstruction*>(inst.get());
                //     auto src = loadInst->getPtr();
                    
                // }
                
            }
        }
    }
}