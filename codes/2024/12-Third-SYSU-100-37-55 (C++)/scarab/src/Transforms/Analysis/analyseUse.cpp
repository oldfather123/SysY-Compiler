#include "analyseUse.h"

bool analyseAble(InsID id) {
    return ((id != Return && id != Br && id != Store && id != Call));
}

void clearUse(Module &ir, bool isGlobal) {
    if(isGlobal){
        for(auto val: ir.globalVariables) {
            val->useCount = 0;
            val->useHead = nullptr;
        }
    }else{
        for(auto func : ir.globalFunctions){
            if(!func->isLibraryFunction){
                for(auto val : func->formalArguments){
                    val->useCount = 0;
                    val->useHead = nullptr;
                }
                for(auto bb: func->basicBlocks) {
                    for(auto instr: bb->instructions) {
                        if(instr->reg) {
                            instr->reg->useCount = 0;
                            instr->reg->useHead = nullptr;
                        }
                    }
                }
            }
        }
    }
}

void analyseUse(Module &ir){
    clearUse(ir, true);
    clearUse(ir, false);

    for(auto func : ir.globalFunctions){
        if(!func->isLibraryFunction){
            for(auto bb: func->basicBlocks) {
                bb->parentFunction = func;
                for(auto instr : bb->instructions) {
                    instr->basicblock = bb;
                
                    if(instr->type == Alloca)
                        continue;
                    if(instr->reg)
                        instr->reg->I = instr.get();
                    int numOperand = instr->getOperandCount();
                    for(int i = 0; i < numOperand; i ++) {
                        if(analyseAble(instr->type))
                            newUse(instr->getOperand(i).get(), instr.get(), instr->reg.get());
                        else
                            newUse(instr->getOperand(i).get(), instr.get());
                    }
                }
            }   
        }
    }
}