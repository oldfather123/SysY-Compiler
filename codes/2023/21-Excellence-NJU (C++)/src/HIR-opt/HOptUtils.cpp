//
// Created by q1w2e3r4 on 23-7-28.
//

#include "HIR-opt/HOptUtils.h"
#include <cstring>
// Replace oldVal with newVal in all usages of oldVal
void replaceAllUsesOf(ValueRef* oldVal, ValueRef* newVal){
    for(auto instr:oldVal->use){
        instr->replace(oldVal,newVal);
    }
    oldVal->use.clear();

    // remove old's def from newVal
    auto it = std::find(newVal->use.begin(), newVal->use.end(), oldVal->get_def());
    if(it != newVal->use.end())
        newVal->use.erase(it);
}

// delete this block, and update all blocks pred/succ it;
void DelBlock(BasicBlock* block){
    // 1. delete all instr
    for(auto instr: block->local_instr){
        instr->deleted = true;
        DelUseOfInstruction(instr);
    }

    // 2. delete itself func's block_list
    auto& v = block->func_belong->block_list;
    for(auto it = v.begin(); it != v.end(); ++it){
        if((*it) == block){
            v.erase(it);
            break;
        }
    }

    // 3. delete itself in all succ blocks' pred ,also clear its own succ
    auto& succ = block->succ;
    for(auto it1 = succ.begin(); it1 != succ.end(); ){
        BasicBlock* next = *it1;
        it1 = succ.erase(it1);
        for(auto it = next->pred.begin(); it != next->pred.end(); ++it){
            if(*it == block){
                next->pred.erase(it);
                break;
            }
        }
        OptPhi(block,next);
        if(next->pred.empty()){
            DelBlock(next);  // and it may be recursive
        }
    }

}

// 删除本指令在ref中的使用(use)
void DelUseOfInstruction(Instruction *instr) {
    // for all use ref of instr ,delete this in ref's use-list
    // Caution: if you want to remove instr from local_instr,use func below this
    for(auto ref:instr->use_list){
        DelUse(ref,instr);
    }
}

// 删除本指令在ref中的使用(use)+此指令的位置，相当于移除整个指令
void DeleteInstruction(Instruction* instr){
    instr->deleted = true;
    DelUseOfInstruction(instr);
    Remove_LocalInstr(instr);
}

// 将指令从当前位置删除，但不删除用法
void Remove_LocalInstr(Instruction *instr) {
    auto& v = dynamic_cast<IRInstruction*>(instr)->block->local_instr;
    auto it = std::find(v.begin(),v.end(),instr);
    if(it != v.end()) {
        v.erase(it);
    }
}

// 删除在ref 的use列表中删除instr
void DelUse(ValueRef* ref, Instruction* instr){
    for(auto it = ref->use.begin(); it != ref->use.end(); ++it){
        if(*it == instr){
            ref->use.erase(it);
            return;
        }
    }
}

// 解绑两个块的联系(PHI指令会对应修改)
void UnlinkBlock(BasicBlock *pred, BasicBlock *succ) {
    OptPhi(pred,succ);

    auto& v1 = pred->succ;
    for(auto it = v1.begin();it != v1.end();++it){
        if(*it == succ){
            v1.erase(it);
            break;
        }
    }

    auto& v2 = succ->pred;
    for(auto it = v2.begin();it != v2.end();++it){
        if(*it == pred){
            v2.erase(it);
            break;
        }
    }

    // if succ doesn't have any preds,it became a dead block;
    if(v2.empty()){
        DelBlock(succ);
    }
}

void OptPhi(BasicBlock* from,BasicBlock* to){
    for(auto instr : to->local_instr){
        auto phi = dynamic_cast<PhiInstruction*>(instr);
        if(phi == nullptr) break;

        phi->use_list.erase(phi->mp[from]);
        auto& v = phi->mp[from]->use;
        auto it = std::find(v.begin(),v.end(), phi);
        if(it != v.end()){
            v.erase(it);
        }

        phi->use_list.erase(from);
        phi->mp.erase(from);
        --phi->branch_cnt;
    }
}

// 原来关系为from -> to,现在在中间插入一个块变成 from -> insert -> to
void Insert_Block(BasicBlock *from, BasicBlock *to, BasicBlock *insert) {
    auto& v1 = from->succ;
    for(auto it = v1.begin(); it != v1.end(); ++it){
        if(*it == to){
            *it = insert;
            break;
        }
    }

    auto &v2 = to->pred;
    for(auto it = v2.begin(); it != v2.end(); ++it){
        if(*it == from){
            *it = insert;
            break;
        }
    }

    for(auto instr:from->local_instr){
        if(instr->instType == BR || instr->instType == CONDBR){
            instr->replace(to,insert);
        }
    }

    for(auto instr:to->local_instr){
        if(instr->instType != PHI) break;
        instr->replace(from,insert);
    }

    if(find(insert->pred.begin(),insert->pred.end(),from) == insert->pred.end()){
        insert->pred.push_back(from);
    }
    if(find(insert->succ.begin(),insert->succ.end(),to) == insert->succ.end()){
        insert->succ.push_back(to);
    }

    if(insert->local_instr.empty()){
        IRInstruction * br = new BrInstruction(to);
        br->block = insert;
        insert->local_instr.push_back(br);
    }
}

// 在一个块的最前面插入指令（同时修改instr所属块的信息（最好是调这个省的你忘了））
void Insert_instr_atFront(Instruction *instr, BasicBlock *block) {
    block->insertCodeAtFront(instr);
    dynamic_cast<IRInstruction*>(instr)->block = block;
}

// 在一个块的最末端插入指令，如果最后一句是跳转，就插在倒数第二句（同时修改instr所属块的信息（最好是调这个省的你忘了））
void Insert_instr_beforeBr(Instruction *instr, BasicBlock *block) {
    dynamic_cast<IRInstruction*>(instr)->block = block;

    if(block->local_instr.empty()){
        block->local_instr.push_back(instr);
        return;
    }

    auto it = block->local_instr.end()-1;
    if((*it)->instType == BR || (*it)->instType == CONDBR){
        block->local_instr.insert(it,instr); //插在跳转语句之前
    }
    else{
        block->local_instr.push_back(instr); // 否则直接放末尾
    }

}

// 只在AlgeOpt用，替换前后两指令def相同
void ReplaceInstr(Instruction *old, Instruction *now) {
    BasicBlock * bb = dynamic_cast<IRInstruction*>(old)->block;
    auto& v = bb->local_instr;
    for(auto & i : v){
        if(i == old){
            i = now;
            break;
        }
    }
    old->deleted = true;
    DelUseOfInstruction(old);
    dynamic_cast<IRInstruction*>(now)->block = bb;
}
