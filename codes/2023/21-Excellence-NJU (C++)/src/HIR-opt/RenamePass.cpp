//
// Created by q1w2e3r4 on 23-8-7.
//

#include "HIR-opt/RenamePass.h"
#include <set>

void RenamePass::run() {
    for(auto& [name,func]: gu->func_table){
        if(func->entry != nullptr) {
            Rename(func);
        }
    }
}

void renameRef(ValueRef * ref){
    ref->name = Utils::getNewTamp();
    if(ref->type == Arr){ // 与之同名的ptr也要因此改名
        for(auto ptr: ((Array*)ref)->linked_ptr){
            ptr->name = ref->name;
        }
    }
}
void RenamePass::Rename(Function *func) {
    Utils::setTempCounter(func->funcType->arguments.size());
    set<ValueRef*> s;
    for(auto block: func->block_list){
        for(auto instr: block->local_instr){
            if(!instr->def_list.empty()){
                ValueRef* def = *(instr->def_list.begin()); // def_list should only have one ref?
                if(def->type != IntConst && def->type != FloatConst
                && def->type != SYMBOL && !s.count(def)){
                   s.insert(def);
                   renameRef(def);
                }
            }
        }
    }
}
