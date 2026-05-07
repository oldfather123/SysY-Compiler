#include "riscv.h"

void useAnalysis(MProg* mProg) {
    for(auto mFunc : mProg->mFuncs) {
        mFunc->clearUseDefMap();
        for(auto mb : mFunc->mBlocks) {
            for(auto I = mb->firInst; I; I = I->next) {
                auto IDefs = IGetDefsPtr(I);
                auto IUses = IGetUsesPtr(I, mFunc->hasReturnValue);
                if(IDefs.empty()) {
                    IDefs.push(nullptr);
                }
                for(auto IDef : IDefs) {
                    for(auto IUse : IUses) {
                        auto newUse = new MIUse(IUse, IDef, I);
                        IUse->IAddUse(newUse, mFunc);
                    }
                    if(IDef != nullptr) {
                        IDef->setDefI(I, mFunc);
                    }
                }
                auto FDefs = FGetDefsPtr(I);
                auto FUses = FGetUsesPtr(I, mFunc->hasReturnValue);
                if(FDefs.empty()) {
                    FDefs.push(nullptr);
                }
                for(auto FDef : FDefs) {
                    for(auto FUse : FUses) {
                        auto newUse = new MIUse(FUse, FDef, I);
                        FUse->FAddUse(newUse, mFunc);
                    }
                    if(FDef != nullptr) {
                        FDef->setDefI(I, mFunc);
                    }
                }
            }
        }
    }
}