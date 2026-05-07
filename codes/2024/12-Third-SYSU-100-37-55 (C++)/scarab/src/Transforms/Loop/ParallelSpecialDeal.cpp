#include "ParallelSpecialDeal.h"

void parallelSpecialDeal(FunctionPtr func, Module &ir){
    for(auto bb:func->basicBlocks){
        vector<pair<InstructionPtr, InstructionPtr>> instToInsert;
        for(auto inst:bb->instructions){
            if(inst->type == Call){
                auto callInst = dynamic_cast<CallInstruction*>(inst.get());
                auto funcName = callInst->func->name.substr(0, 12);
                if(funcName == "ParallelLoop"){
                    auto funcCastInst = make_shared<FuncCastInstruction>(callInst->func->name, bb);
                    //bb->insertInstructionBefore(funcCastInst, inst);
                    callInst->func = ir.globalFunctions[14]; // 14是cmmcParallelFor
                    //break;
                    instToInsert.push_back({funcCastInst, inst});
                }
            }
        }
        for(auto it:instToInsert){
            bb->insertInstructionBefore(it.first, it.second);
        }
    }
}