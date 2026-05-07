//
// Created by q1w2e3r4 on 23-7-27.
//

#include "HIR-opt/Mem2RegPass.h"
#include "IRInstruction.h"
#include "HIR-opt/HOptUtils.h"
#include<map>
#include<set>
#include<stack>
using namespace std;

// insertPhi uses
map<ValueRef*,set<BasicBlock*>> defsites; // describe what blocks def this ref
map<BasicBlock*,set<ValueRef*>> placed; // describe what ref need to be phi in one block
Undef* undef = new Undef();
// renamePhi uses
map<ValueRef*,stack<ValueRef*>> st;// stack top = live var(we use it to replace symbol)

// del use in load/store
void DelUseinLS(ValueRef* ref){
    for(auto it = ref->use.begin(); it != ref->use.end();){
        if((*it)->instType == LOAD || (*it)->instType == STORE){
            it = ref->use.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Mem2RegPass::run() {
    for(auto& [name,func] : globalUnit->func_table){
        if(func->dom_root != nullptr) {
            st.clear();
            Utils::labelCounter = 0;
            insertPhi(func);
            renamePhi(func->dom_root);
            mergeEntry(func);
        }
    }
}

void Mem2RegPass::insertPhi(Function *func) {
    for(auto block:func->block_list){ // cal defsites
        for(auto instr:block->local_instr){
            if(instr->def_list.empty()) continue;
            ValueRef * var = *(instr->def_list.begin());
            if(var->type == SYMBOL && !dynamic_cast<Symbol*>(var)->is_global)
                defsites[var].insert(block);
        }
    }

    for(auto& [var,defs]: defsites){
        bool is_int = ((Symbol*)var)->symbolType->type == INT32TYPE;
        while(!defs.empty()){
            BasicBlock* block = *(defs.begin()); defs.erase(defs.begin());
            auto DF = block->domNode->DF;
            for(auto df : DF){
                BasicBlock* bb = df->bb;
                if(!placed[bb].count(var)){
                    ValueRef* result;
                    if(is_int)
                        result = new Int_Var("%phi_");
                    else
                        result = new Float_Var("%phi_");

                    IRInstruction * phi = new PhiInstruction(var,result,bb->pred.size());
                    Insert_instr_atFront(phi,bb);
                    placed[bb].insert(var);

                    if(!bb->live_def.count(var)){
                        defsites[var].insert(bb);
                    }
                }
            }
        }
    }
}

void Mem2RegPass::renamePhi(DomNode * root) {
    map<ValueRef*,int> counter; // mark how many times one var is defined(push into stack) in this block
    BasicBlock* nowBlock = root->bb;
    auto& instructions = nowBlock->local_instr;
    for(auto it = instructions.begin(); it != instructions.end();){
        if((*it)->instType == LOAD){ // use
            // load src -> dst,we want to delete this LOADinstr, so replace dst
            ValueRef* src = ((LoadInstruction*)(*it))->src;
            ValueRef* dst = ((LoadInstruction*)(*it))->dst;
            if(src->type == Ptr || dynamic_cast<Symbol*>(src) -> is_global) { // we don't simplify global symbol and array/ptr;
                ++it;
                continue;
            }
            ValueRef* replaced = st[src].top();
            replaceAllUsesOf(dst, replaced);

            it = instructions.erase(it);
        }
        else if((*it)->instType == STORE) { // def
            ValueRef* src = ((StoreInstruction*)(*it))->src;
            ValueRef* dst = ((StoreInstruction*)(*it))->dst;
            // store src -> dst,  we record src as def to replace other uses

            if(dst->type == Ptr || dynamic_cast<Symbol*>(dst) -> is_global) { // we don't simplify global symbol and array/ptr;
                ++it;
                continue;
            }
            st[dst].push(src); counter[dst]++;
            DelUseinLS(src);
            it = instructions.erase(it);
        }
        else if((*it)->instType == PHI){ //
            ValueRef* var = ((PhiInstruction*)(*it))->symbol;
            ValueRef* value = ((PhiInstruction*)(*it))->result;
            st[var].push(value); counter[var]++;
            ++it;
        }
        else{
            ++it;
        }
    }

    for(auto next: nowBlock->succ){ // rename phi node
        for(auto instr: next->local_instr){
            if(instr->instType != PHI) break;
            auto phi = (PhiInstruction*)instr;
            ValueRef* var = phi->symbol;
            if(st[var].empty()){
                phi->mp[nowBlock] = undef;
            }
            else {
                phi->mp[nowBlock] = st[var].top();
                phi->addUse(st[var].top());
            }
            phi->addUse(nowBlock);
        }
    }

    for(auto child: root->child){ // recursive-call
        renamePhi(child);
    }

    for(auto&[var,cnt]:counter){
        for(int i=0;i<cnt;++i)
            st[var].pop();
    }
}

void Mem2RegPass::mergeEntry(Function *func) {
    // just delete entry block and all non-arr alloc instr in it;
    BasicBlock* entry = func->entry; // this block only has allca instr;
    BasicBlock* next = entry->succ[0]; // real entry block
    auto& v = entry->local_instr;
    stack<Instruction*> stk;
    for(auto it = v.begin();;){
        auto instr = dynamic_cast<AllocaInstruction*>(*it);
        if(instr == nullptr)  // last instr(jump to next block)
            break;
        if(instr->varType->type == ARRAYTYPE) { // if arr alloc instr, move it to next block
            stk.push(*it);
        }
        it = v.erase(it);
    }

    while(!stk.empty()){
        Insert_instr_atFront(stk.top(),next);
        stk.pop();
    }

    //finally delete this entry block;
    next->pred.clear();
    func->block_list.erase(func->block_list.begin());
    func->entry = next;
}
