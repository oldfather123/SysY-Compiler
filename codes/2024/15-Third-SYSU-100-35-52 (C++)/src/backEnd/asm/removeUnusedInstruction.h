#include "riscv.h"

bool IMaybeUnused(MITag kind) {
    return kind == MI_BINARY || kind == MI_LOAD || kind == MI_MOVE;
}

bool FMaybeUnused(MITag kind) {
    return kind == MI_FBINARY || kind == MI_FLOAD || kind == MI_FMOVE;
}

void removeUnusedInstruction(MProg* mProg) {
    for(auto mFunc : mProg->mFuncs) {
        curFunc = mFunc;
        for(auto mb : curFunc->mBlocks) {
            for(auto inst = mb->firInst; inst; inst = inst->next) {
                if(IMaybeUnused(inst->tag)) {
                    auto defs = IGetDefs(inst);
                    bool unused = defs.size() > 0;
                    for(auto def : defs) {
                        if(def.IGetUseHead(curFunc) != nullptr || def.tag == IREG) {
                            unused = false;
                            break;
                        }
                    }
                    if(unused) {
                        inst->deleteFromParent(curFunc);
                        stillOptimize = true;
                    }
                } else if(FMaybeUnused(inst->tag)) {
                    auto defs = FGetDefs(inst);
                    bool unused = defs.size() > 0;
                    for(auto def : defs) {
                        if(def.FGetUseHead(curFunc) != nullptr || def.tag == FREG) {
                            unused = false;
                            break;
                        }
                    }
                    if(unused) {
                        inst->deleteFromParent(curFunc);
                        stillOptimize = true;
                    }
                }
            }
        }
    }
}
