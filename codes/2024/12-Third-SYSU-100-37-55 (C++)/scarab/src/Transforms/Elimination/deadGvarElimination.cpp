#include "deadGvarElimination.h"

void deadGvarElimnation(Module &ir){
    vector<VariablePtr> newGvar;
    for(auto gvar : ir.globalVariables){
        unordered_set<InstructionPtr>isRelated;

        bool isUseFul = false;
        auto use = gvar->useHead;
        stack<InstructionPtr>worklist;
        while (use){
            worklist.push(use->user->getSharedThis());
            use = use->next;
        }
        
        while(!worklist.empty() && !isUseFul){
            auto ins = worklist.top();
            worklist.pop();

            if(ins == nullptr){
                use = use->next;
                continue;
            }
            isRelated.insert(ins);

            switch (ins->type)
            {
            case Call:{
                isUseFul = true;
                break;
            }
            case Store:{
                break;
            }
            case Load:{
                isUseFul = true;
                break;
            }
            case Return:{
                isUseFul = true;
                break;
            }
            case GEP:{
                if(ins->reg){
                    auto gepHead =ins->reg->useHead;

                    while(gepHead){
                        auto gepIns = gepHead->user->getSharedThis();
                        worklist.push(gepIns);
                        gepHead = gepHead->next;
                    }
                }
                break;
            }
            
            default:
                break;
            }
        }

        if(isUseFul){
            newGvar.push_back(gvar);
        }else{
            for(auto func : ir.globalFunctions){
                if(func->isLibraryFunction)
                    continue;
                for(auto bb : func->basicBlocks){
                    vector<InstructionPtr>newIns;
                    for(auto ins : bb->instructions){
                        if(isRelated.count(ins) == 0){
                            newIns.push_back(ins);
                        }
                    }
                    
                    bb->instructions = newIns;
                }

            }
        }

        
    }

    ir.globalVariables = newGvar;
}