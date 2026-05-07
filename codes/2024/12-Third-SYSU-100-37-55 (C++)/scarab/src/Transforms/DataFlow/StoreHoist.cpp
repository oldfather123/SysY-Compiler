#include "StoreHoist.h"

void findHoist(LoopPtr loop){
    for(auto bb : loop->basicBlocks){
        vector<InstructionPtr> instToHoist;
        for(auto inst:bb->instructions){
            if(inst->type == Store){
                auto storeInst = dynamic_cast<StoreInstruction*>(inst.get());
                if(storeInst->des->name.substr(0, 15) == "parallelForBody"){
                    auto val = storeInst->value;
                    bool flag = false;
                    if(!val->I){
                        flag = true;
                    }
                    else{
                        auto valInst = val->I;
                        if(!loop->contains(valInst->basicblock)){
                            flag = true;
                        }
                    }
                    if(flag){
                        instToHoist.push_back(inst);
                    }
                }
            }
        }
        for(auto it:instToHoist){
            bb->removeInstruction(it);
            loop->preHeader->insertInstructionBefore(it, loop->preHeader->terminator);
        }
    }
}

void storeHoist(FunctionPtr func){
    queue<LoopPtr> workList;
    for(auto loop:func->loopList){
        if(loop->subLoops.size()==0){
            workList.push(loop);
        }
    }
    while(!workList.empty()){
        auto loop = workList.front();
        workList.pop();
        if(loop->parentLoop){
            workList.push(loop->parentLoop);
        }
        findHoist(loop);
    }
}