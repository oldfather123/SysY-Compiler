#include "loadStoreMerge.h"

void setEntryBlockFlag(FunctionPtr func){
    func->getEntryBlock()->isEntryBlock = true;;
}

// 宗旨是在每个块内部，store后面不会再有load指令（因为可以直接传下来）
// 随后再通过BFS的方式传递值
void loadStoreMerge(Module &ir){
    for(auto it:ir.globalFunctions){
        if(it->isLibraryFunction) continue;
        setEntryBlockFlag(it);
    }
    unordered_set<ValuePtr> globalSingleVar;
    unordered_map<ValuePtr, unordered_map<BasicBlockPtr, ValuePtr>> valueMap;
    for (auto &var : ir.globalVariables){
        auto type = var->type;
        bool is_int = type->isInt();
        bool is_float = type->isFloat();
        bool is_bool = type->isBool();
        if(var->type->ID == TypeID::ArrID || var->type->ID == TypeID::PtrID){
            continue;
        }
        globalSingleVar.insert(var);
    }
        
    unordered_set<InstructionPtr> instToErase;
    unordered_set<BasicBlockPtr> changedBlock;
    for (auto &func : ir.globalFunctions){
        if (func->isLibraryFunction) continue;
        for(auto& bb : func->basicBlocks){
            for (auto &ins : bb->instructions){
                if(auto Si = dynamic_cast<StoreInstruction*>(ins.get())){
                    if(Si->des == nullptr) continue;
                    if(globalSingleVar.count(Si->des)){
                        valueMap[Si->des][bb] = Si->value;
                    }
                }
                if(auto Li = dynamic_cast<LoadInstruction*>(ins.get())){
                    if(Li->from == nullptr) continue;
                    if(globalSingleVar.count(Li->from)){
                        if(valueMap[Li->from].count(bb)){
                            auto theValue = valueMap[Li->from][bb];
                            replaceVarByVar(Li->reg, theValue);
                            instToErase.insert(ins);
                            changedBlock.insert(Li->basicblock);
                            deleteUser(Li->reg);
                        }
                    }
                }
            }
        }
    }

    for(auto &var:globalSingleVar){
        queue<BasicBlockPtr> worklist;
        unordered_map<BasicBlockPtr, ValuePtr> acrossBlockValue;
        unordered_set<BasicBlockPtr> propBlock;
        bool entryCheck = false;
        for(auto it: valueMap[var]){
            worklist.push(it.first);
            acrossBlockValue[it.first] = it.second;
            if(it.first->isEntryBlock){
                entryCheck = true;
            }
        }
        if(worklist.empty())continue;
        if(!entryCheck){
            worklist.push(worklist.front()->parentFunction->getEntryBlock());
            acrossBlockValue[worklist.front()->parentFunction->getEntryBlock()] = Const::createConstant(Type::getInt(), 0);
        }
        
        while(!worklist.empty()){
            auto bb = worklist.front();
            worklist.pop();
            auto bbValue = acrossBlockValue[bb];
            for(auto succ: bb->succBasicBlocks){
                if(propBlock.count(succ)){
                    if(acrossBlockValue[succ] == nullptr){

                    }
                    else if(acrossBlockValue[succ] != bbValue){
                        acrossBlockValue[succ] = nullptr;
                        worklist.push(succ);
                        propBlock.insert(succ);
                    }
                    else if(acrossBlockValue[succ] == bbValue){

                    }
                }
                else{
                    acrossBlockValue[succ] = bbValue;
                    worklist.push(succ);
                    propBlock.insert(succ);
                }
            }
        }

        for(auto it : acrossBlockValue){
            if(it.second != nullptr && propBlock.count(it.first)){
                bool flag = true;
                ValuePtr sameVal = nullptr;
                for(auto pred : it.first->predBasicBlocks){
                    if(!propBlock.count(pred)){
                        flag = false;
                        break;
                    }
                    if(sameVal == nullptr){
                        sameVal = acrossBlockValue[pred];
                    }
                    if(sameVal!=acrossBlockValue[pred]){
                        flag = false;
                        break;
                    }
                }
                if(!flag) continue;
                for(auto inst : it.first->instructions){
                    if(auto Li = dynamic_cast<LoadInstruction*>(inst.get())){
                        if(Li->from == nullptr) continue;
                        if(Li->from == var){
                            replaceVarByVar(Li->reg, it.second);
                            instToErase.insert(inst);
                            changedBlock.insert(Li->basicblock);
                            deleteUser(Li->reg);
                        }
                    }
                }
            }
        }
    }

    for(auto newBB: changedBlock) {
        vector<InstructionPtr > newIns;
        for(int i = 0; i < newBB->instructions.size(); i++){
            if(!instToErase.count(newBB->instructions[i])){
                newIns.push_back(newBB->instructions[i]);
            }
        }
        newBB->instructions = newIns;
    }
}