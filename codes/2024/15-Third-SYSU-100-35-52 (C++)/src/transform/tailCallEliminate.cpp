#include "tailCallEliminate.h"
#include "Instruction.h"



// TODO: to test
static bool isRecCall(Instruction* inst, FunctionPtr func) {
    if (inst->insId != InsID::Call) return false;
    auto callInst = dynamic_cast<CallInstruction*>(inst);
    auto callee = callInst->getCallee();
    return callee = func;
}




bool runTailCallEliminate(FunctionPtr func) {
    // require no alloc ???? why
    if (func->getEntryBlock()->instructionsRef().front()->insId == InsID::Alloca) return false;
    for (auto& bb: func->basicBlocks) {
        auto firstRecCall = bb->instructionsRef().end();
        for (auto iter = bb->instructionsRef().begin(); iter != bb->instructionsRef().end(); iter++) {
            if (isRecCall(iter->get(), func)) {
                firstRecCall = iter;
                break;
            }
        }
        if (firstRecCall != bb->instructionsRef().end()) {
            for (auto it = firstRecCall; it != bb->instructionsRef().end(); it++) {
                auto inst = it->get();
                if (inst->canbeOperand()) {
                    // why inst replaced with undifined value
                    //inst->replaceWith(ValuePtr value)
                }
            }
            
        }

    }





    return false;
}