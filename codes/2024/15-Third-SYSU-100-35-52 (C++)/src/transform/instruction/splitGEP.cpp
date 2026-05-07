#include "splitGEP.h"

void splitGEP(Module& mod) {
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        for(auto& bb : func->getBasicBlocks()) {
            for(auto& inst : bb->instructions()) {
                if(auto gepInst = inst->as<GetElementPtrInstruction>()) {
                    if (gepInst->getNumOperands() <= 3) {
                        continue;
                    }
                    auto lastBase = gepInst->getBase();
                    for(auto idx : gepInst->getArrIdx()) {
                        auto newGep = new GetElementPtrInstruction(lastBase, { Const::getConst(Type::getInt64(), 0), idx });
                        newGep->setLabel("newGEP");
                        auto iter = std::find_if(bb->instructionsRef().begin(), bb->instructionsRef().end(),
                                                 [&](auto& inst) { return inst.get() == gepInst; });
                        insertBefore(bb.get(), iter, newGep);
                        lastBase = newGep;
                    }
                    gepInst->replaceWith(lastBase);
                }
            }
        }
    }
}