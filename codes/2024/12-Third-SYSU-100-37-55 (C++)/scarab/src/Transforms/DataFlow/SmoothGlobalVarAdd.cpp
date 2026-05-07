#include "SmoothGlobalVarAdd.h"

void hoistGlobalVar(LoopPtr loop){
    if(!checkLegalLoop_partial(loop)) return;
    auto header = loop->header;
    auto latch = loop->latchBlock;
    unordered_map<ValuePtr, pair<Instruction*, int>> varToHoist;
    for(int i = 0; i < latch->instructions.size(); i++){
        auto inst = latch->instructions[i];
        if(inst->type == Store){
            auto storeInst = dynamic_cast<StoreInstruction*>(inst.get());
            auto storeDesVar = dynamic_cast<Variable*>(storeInst->des.get());
            if(storeDesVar && storeDesVar->isGlobal){
                auto currentVal = storeInst->value;
                while(1){
                    auto currentInst = currentVal->I;
                    if(currentInst == nullptr) break;
                    if(currentInst->type == Binary){
                        auto currentBinary = dynamic_cast<BinaryInstruction*>(currentInst);
                        if(currentBinary->op != '+') break;
                        currentVal = currentBinary->b;
                        continue;
                    }
                    else if(currentInst->type == Load){
                        auto currentLoad = dynamic_cast<LoadInstruction*>(currentInst);
                        auto currentLoadVar = dynamic_cast<Variable*>(currentLoad->from.get());
                        auto useHead = currentInst->reg->useHead;
                        int useCount = 0;
                        while (useHead) {
                            auto use = useHead->user;
                            useCount++;
                            useHead = useHead->next;
                        }
                        if(useCount > 1) break; // 说明有别的地方还用了
                        if(currentLoadVar == storeDesVar){
                            varToHoist[storeInst->des] = {currentInst, i};
                            break;
                        }
                    }
                    else break;
                }
            }
        }
    }
    for(auto it: varToHoist){
        auto globalVar = it.first;
        auto loadI = it.second.first;
        InstructionPtr loadInst;
        for(int i = 0; i < latch->instructions.size(); i++){
            if(latch->instructions[i].get() == loadI){
                loadInst = latch->instructions[i];
            }
        }
        int storeIndex = it.second.second;
        auto storeInst = dynamic_cast<StoreInstruction*>(latch->instructions[storeIndex].get());
        auto newPhiNode = shared_ptr<PhiInstruction>(new PhiInstruction(header, globalVar->type));
        newPhiNode->specialTag = true;
        if(globalVar->type->ID == IntID){
            newPhiNode->addFrom(Const::createConstant(Type::getInt(), int(0)), loop->preHeader);
        }
        else if(globalVar->type->ID == FLoatID){
            newPhiNode->addFrom(Const::createConstant(Type::getFloat(), float(0.0)), loop->preHeader);
        }
        newPhiNode->addFrom(storeInst->value, latch);
        header->instructions.insert(header->instructions.begin(), newPhiNode);
        replaceVarByVar(loadInst->reg, newPhiNode->reg);
        removeAllOperandUses(loadInst);

        vector<InstructionPtr> latchNewInstructions;
        for(int i = 0; i < latch->instructions.size(); i++){
            if(latch->instructions[i] == loadInst || i == storeIndex) continue;
            latchNewInstructions.push_back(latch->instructions[i]);
        }
        latch->instructions = latchNewInstructions;
        
        auto exitBlock = *loop->exitBlocks.begin();
        vector<InstructionPtr> newInstructions;
        int stopIndex = 0;
        for(int i=0;i<exitBlock->instructions.size();i++){
            if(exitBlock->instructions[i]->type == Phi)newInstructions.push_back(exitBlock->instructions[i]);
            else {
                stopIndex = i;
                break;
            }
        }
        auto newLoadInst = shared_ptr<LoadInstruction>(new LoadInstruction(globalVar, exitBlock->parentFunction->createRegister(it.first->type), exitBlock));
        newInstructions.push_back(newLoadInst);
        auto newBinaryInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(newPhiNode->reg, newLoadInst->reg, '+', exitBlock));
        newInstructions.push_back(newBinaryInst);
        auto newStoreInst = shared_ptr<StoreInstruction>(new StoreInstruction(globalVar, newBinaryInst->reg, exitBlock));
        newInstructions.push_back(newStoreInst);
        for(int i = stopIndex; i<exitBlock->instructions.size();i++){
            newInstructions.push_back(exitBlock->instructions[i]);
        }
        exitBlock->instructions = newInstructions;

    }
}

void SmoothGlobalVarAdd(FunctionPtr func){
    for(auto loop : func->loopList){
        if(loop->subLoops.size()) continue;
        hoistGlobalVar(loop);
    }
}