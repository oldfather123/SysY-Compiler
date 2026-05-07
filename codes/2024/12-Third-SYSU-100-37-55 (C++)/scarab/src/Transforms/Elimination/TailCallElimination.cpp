#include "TailCallElimination.h"

bool checkTailCall(FunctionPtr func){
    int callCnt=0;
    for(auto bb: func->basicBlocks){
        int n=bb->instructions.size();
        int inBlockCallCnt=0;
        for(int i=0;i<n;i++){
            auto inst = bb->instructions[i];
            if(inst->type == Call){
                callCnt++;
                inBlockCallCnt++;
                auto callInst = dynamic_cast<CallInstruction*>(inst.get());
                if(callInst->func != func) return false;
                auto nextInst = bb->instructions[i+1];
                if(nextInst->type != Br) return false;
                auto brInst = dynamic_cast<BrInstruction*>(nextInst.get());
                if(brInst->exp || brInst->label2 || brInst->label1->name != "return") return false;
            }
        }
        if(inBlockCallCnt>1) return false;
    }
    return callCnt > 0 && !func->returnValue->type->isVoid() && func->returnBasicBlock->instructions.size()==2 && func->formalArguments.size()<5;
}

void eliminateTailCall(FunctionPtr func){
    auto entry = func->getEntryBlock();
    auto newEntry = make_shared<BasicBlock>(std::make_shared<Label>("newEntry"));
    auto br2Entry = make_shared<BrInstruction>(func->getEntryBlock()->label, newEntry);
    newEntry->appendInstruction(br2Entry);
    newEntry->setTerminator(br2Entry);
    func->basicBlocks.insert(func->basicBlocks.begin(), newEntry);
    //fakeEntry->instructions = func->getEntryBlock()->instructions;
    vector<InstructionPtr> argPhi;
    vector<InstructionPtr> entryNewInstructions;
    for(auto it : func->formalArguments){
        auto phiNode = make_shared<PhiInstruction>(entry, it->type);
        argPhi.push_back(phiNode);
        replaceVarByVar(it, phiNode->reg);
        phiNode->addFrom(it, newEntry);
        entryNewInstructions.push_back(phiNode);
    }

    int returnReg = 0;
    auto returnBlock = func->returnBasicBlock;
    auto returnPhi = dynamic_cast<PhiInstruction*>(returnBlock->instructions[0].get());
    vector<std::pair<ValuePtr,shared_ptr<BasicBlock>>> newFrom;
    for(auto it:returnPhi->from){
        for(int i=0;i<argPhi.size();i++){
            if(it.first == argPhi[i]->reg){
                //replaceVarByVar(returnPhi->reg, argPhi[i]->reg);
                //newFrom.push_back({argPhi[i]->reg, entry});
                returnReg = i;
            }
        }
    }

    // for(auto it:returnPhi->from){
    //     if(it.first->isConst){
    //         // auto phiII = dynamic_cast<PhiInstruction*>(phiI.get());
    //         // phiII->addFrom(phiII->reg, it.second);
    //         newFrom.push_back({argPhi[returnReg]->reg, it.second});
    //         auto theInst = it.second->instructions[0];
    //         auto theBr = dynamic_cast<BrInstruction*>(theInst.get());
    //         theBr->label1 = returnBlock->label;
    //     }
    // }
    
    
    //returnBlock->instructions={returnBlock->instructions.back()};

    for(auto inst : entry->instructions) entryNewInstructions.push_back(inst);
    entry->instructions = entryNewInstructions;
    
    for(auto bb:func->basicBlocks){
        int n=bb->instructions.size();
        // if(n==1 && bb->instructions[0]->type == Br && bb->label->name != "newEntry"){
            
        // }
        vector<InstructionPtr> newInst;
        for(int i=0;i<n;i++){
            auto inst = bb->instructions[i];
            if(inst->type == Call){
                
                auto callInst = dynamic_cast<CallInstruction*>(inst.get());
                assert(i+1<n);
                auto nextBr = dynamic_cast<BrInstruction*>(bb->instructions[i+1].get());
                assert(nextBr);
                nextBr->label1 = entry->label;
                for(int i=0;i<callInst->argv.size();i++){
                    auto argInst = argPhi[i];
                    auto phiNode = dynamic_cast<PhiInstruction*>(argInst.get());
                    phiNode->addFrom(callInst->argv[i], bb);
                }
            }
            else {
                newInst.push_back(inst);
                
                if(inst->type == Br){
                    auto bbr = dynamic_cast<BrInstruction*>(inst.get());
                    if(bbr->label1->name == "return"){
                        newFrom.push_back({argPhi[returnReg]->reg, bb});
                        
                        bbr->label1 = returnBlock->label;
                        continue;
                    }
                }
                
            }
        }
        bb->instructions=newInst;
    }
    for(auto it:returnPhi->from){
        int n=newFrom.size();
        for(int i=0;i<n;i++){
            if(it.second == newFrom[i].second){
                newFrom[i].first = it.first;
                break;
            }
        }
    }
    returnPhi->from = newFrom;
    
}

bool TailCallElimination(FunctionPtr func){
    if(!checkTailCall(func))return false;
    eliminateTailCall(func);
    return true;
}