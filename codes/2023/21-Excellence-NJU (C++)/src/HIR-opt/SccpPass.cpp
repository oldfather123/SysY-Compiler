//
// Created by divin on 23-8-5.
//

#include "IRInstruction.h"
#include "HIR-opt/SccpPass.h"
#include "HIR-opt/HOptUtils.h"
#include "HIR-opt/DomTreePass.h"
#include <queue>

queue<Instruction *> q;

queue<ValueRef *> work_list;


// convert condBr instr -> Br instr
void Cvt_CondBr(CondBrInstruction *instr, bool cond);

// use it if you want to replace ref(note that you should replace one ref ONLY ONCE!!)
void replace_it(ValueRef *old, ValueRef *now);

// use it to remove all connections with refs(def/use), also, remove it from block
void delete_it(Instruction * instr);

void remove_block_from_blocklist(BasicBlock* blk, Function* func) {
    auto &v = func->block_list;
    auto it = std::find(v.begin(),v.end(),blk);
    if(it != v.end()){
        v.erase(it);
    }
}

void SccpPass::instrProp(Instruction *instr) {
    if(instr->deleted) return;
    InstType_Enum instType = instr->instType;
//    instr->outPut(std::cerr);
    switch (instType) {
        case BINARY: {
            auto _instr = dynamic_cast<BinaryInstruction *>(instr);
            if (_instr->src1->type == IntConst && _instr->src2->type == IntConst) {
                Int_Const *cnst;
                switch (_instr->opTy) {
                    case ADD: {
                        cnst = new Int_Const(dynamic_cast<Int_Const *>(_instr->src1)->value +
                                             dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case SUB: {
                        cnst = new Int_Const(dynamic_cast<Int_Const *>(_instr->src1)->value -
                                             dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case MUL: {
                        cnst = new Int_Const(dynamic_cast<Int_Const *>(_instr->src1)->value *
                                             dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case DIV: {
                        cnst = new Int_Const(dynamic_cast<Int_Const *>(_instr->src1)->value /
                                             dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case MOD: {
                        cnst = new Int_Const(dynamic_cast<Int_Const *>(_instr->src1)->value %
                                             dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    default:
                        break;
                }
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            } else if (_instr->src1->type == FloatConst && _instr->src2->type == FloatConst) {
                    Float_Const *cnst;
                    switch (_instr->opTy) {
                        case ADD: {
                            cnst = new Float_Const(dynamic_cast<Float_Const *>(_instr->src1)->value +
                                                 dynamic_cast<Float_Const *>(_instr->src2)->value);
                            replace_it(_instr->dst, cnst);
                            delete_it(instr);
                            break;
                        }
                        case SUB: {
                            cnst = new Float_Const(dynamic_cast<Float_Const *>(_instr->src1)->value -
                                                 dynamic_cast<Float_Const *>(_instr->src2)->value);
                            replace_it(_instr->dst, cnst);
                            delete_it(instr);
                            break;
                        }
                        case MUL: {
                            cnst = new Float_Const(dynamic_cast<Float_Const *>(_instr->src1)->value *
                                                 dynamic_cast<Float_Const*>(_instr->src2)->value);
                            replace_it(_instr->dst, cnst);
                            delete_it(instr);
                            break;
                        }
                        case DIV: {
                            cnst = new Float_Const(dynamic_cast<Float_Const *>(_instr->src1)->value /
                                                 dynamic_cast<Float_Const *>(_instr->src2)->value);
                            replace_it(_instr->dst, cnst);
                            delete_it(instr);
                            break;
                        }
                        default:
                            break;
                    }
            }
            break;
        }
        case CMP: {
            auto *_instr = dynamic_cast<CmpInstruction *>(instr);
            if (_instr->src1->type == IntConst && _instr->src2->type == IntConst) {
                Bool_Const *cnst;
                switch (_instr->opTy) {
                    case EQ: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value ==
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case NE: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value !=
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case LT: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value <
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case LE: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value <=
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case GT: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value >
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                        break;
                    }
                    case GE: {
                        cnst = new Bool_Const(dynamic_cast<Int_Const *>(_instr->src1)->value >=
                                              dynamic_cast<Int_Const *>(_instr->src2)->value);
                    }
                }
                replace_it(_instr->result, cnst);
                delete_it(instr);
            } else if (_instr->src1->type == FloatConst && _instr->src2->type == FloatConst) {
                Bool_Const *cnst;
                switch (_instr->opTy) {
                    case EQ: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value ==
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                        break;
                    }
                    case NE: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value !=
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                        break;
                    }
                    case LT: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value <
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                        break;
                    }
                    case LE: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value <=
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                        break;
                    }
                    case GT: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value >
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                        break;
                    }
                    case GE: {
                        cnst = new Bool_Const(dynamic_cast<Float_Const *>(_instr->src1)->value >=
                                              dynamic_cast<Float_Const *>(_instr->src2)->value);
                    }
                }
                replace_it(_instr->result, cnst);
                delete_it(instr);
            }
            break;
        }
        case CONDBR: {
            auto _instr = dynamic_cast<CondBrInstruction *>(instr);
            if(_instr->condition->type == BoolConst){
                Cvt_CondBr(_instr,((Bool_Const*)_instr->condition)->value);
            }
            break;
        }
        case CALL: {
            auto _instr = dynamic_cast<CallInstruction *>(instr);
            if(_instr->retVal == nullptr) break;

            if (_instr->retVal->type == IntConst) {
                //TODO:当且仅当函数有固定返回值，且无副作用时，可以优化
            } else if (_instr->retVal->type == FloatConst) {

            }
            break;
        }
        case ZEXT: {
            auto _instr = dynamic_cast<ZExtInstruction *>(instr);
            if (_instr->src->type == BoolConst) {
                Int_Const *cnst = dynamic_cast<Bool_Const *>(_instr->src)->value ? new Int_Const(1) : new Int_Const(0);
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            }
            break;
        }
        case XOR: {
            auto _instr = dynamic_cast<XorInstruction *>(instr);
            if (_instr->src->type == BoolConst) {
                auto cnst = new Bool_Const(!dynamic_cast<Bool_Const *>(_instr->src)->value);
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            }
            break;
        }
        case PHI: {
            auto _instr = dynamic_cast<PhiInstruction *>(instr);
            if (_instr->branch_cnt == 1) {
                replace_it(_instr->result,(_instr->mp.begin())->second);
                delete_it(instr);
            }
            else{
                ValueRef * first = (_instr->mp.begin())->second;
                if(first->type == IntConst){
                    int val = ((Int_Const*) first) -> value;
                    for(auto &[block, ref]: _instr->mp){
                        auto cnst = dynamic_cast<Int_Const*>(ref);
                        if(cnst == nullptr || cnst->value != val) return;
                    }
                    replace_it(_instr->result,first);
                    delete_it(instr);
                }
                else if(first->type == FloatConst){
                    double val = ((Float_Const*) first) -> value;
                    for(auto &[block, ref]: _instr->mp){
                        auto cnst = dynamic_cast<Float_Const*>(ref);
                        if(cnst == nullptr || cnst->value != val) return;
                    }
                    replace_it(_instr->result,first);
                    delete_it(instr);
                }
                else{
                    for(auto &[block, ref]: _instr->mp){
                        if(ref != first) return;
                    }
                    replace_it(_instr->result,first);
                    delete_it(instr);
                }
            }
            break;
        }
        //这三个用了可能出问题
        case ITFP: {
            auto _instr = dynamic_cast<ITFPInstruction *>(instr);
            if (_instr->src->type == IntConst) {
                auto cnst = new Float_Const((float) dynamic_cast<Int_Const *>(_instr->src)->value);
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            }
            break;
        }
        case FPTI: {
            auto _instr = dynamic_cast<FPTIInstruction *>(instr);
            if (_instr->src->type == FloatConst) {
                auto cnst = new Int_Const((int) dynamic_cast<Float_Const *>(_instr->src)->value);
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            }
            break;
        }
        case FNEG: {
            auto _instr = dynamic_cast<FNegInstruction *>(instr);
            if (_instr->src->type == FloatConst) {
                auto cnst = new Float_Const(-1 * dynamic_cast<Float_Const *>(_instr->src)->value);
                replace_it(_instr->dst, cnst);
                delete_it(instr);
            }
            break;
        }
        case LOAD:
        case STORE:
        case GEP:
        case RET:
        case ALLOCA:
        case BR:
            break;
    }

}

void SccpPass::constProp(Function *func) {
    for (auto BB: func->block_list) {
        for (auto instr: BB->local_instr) {
            instrProp(instr);
            while (!q.empty()) {
                instrProp(q.front());
                q.pop();
            }
        }
    }
}

void SccpPass::calc(Function *func) {
    for (auto BB : func->block_list) {
        for (auto inst : BB->local_instr) {
            for (auto var : inst->def_list) {
                work_list.push(var);
            }
        }
    }
}

void SccpPass::deadCodeEli(Function *func) {
    calc(func);
    while (!work_list.empty()) {
        auto var = work_list.front();
        work_list.pop();
//        cerr << var->name << endl;
        if (var->use.empty()) {
            if(var->type == IntConst || var->type == FloatConst || var->type == BoolConst || var->type == Undefined || var->type == SYMBOL || var->type == Block) continue;
            if(var->type == Arr){
                bool flag = false;
                for(auto ptr: dynamic_cast<Array*>(var)->linked_ptr){
                    if(!ptr->use.empty()) flag = true;
                }
                if(flag) continue;
            }
            //TODO: Ptr/Arr type shouldn't be ignored
            auto def_inst = var->def.front();
            if(def_inst->deleted) continue;
            if (def_inst->instType != CALL) {//语句没有副作用
                for (auto use_var : def_inst->use_list) {
                    for (auto it = use_var->use.begin(); it != use_var->use.end(); ++it) {
                        if (*it == def_inst) {
                            use_var->use.erase(it);
                            break;
                        }
                    }
                    work_list.push(use_var);
                }
                delete_it(def_inst);
            }
        }
    }
}



BasicBlock* SccpPass::gen_rcfg(Function *func) {
    BasicBlock* rentry = nullptr;
    vector<BasicBlock*> end;
    for (auto bb : func->block_list) {
        if (bb->succ.empty())
            end.push_back(bb);
        auto tmp = bb->pred;
        bb->pred = bb->succ;
        bb->succ = tmp;
    }

    if (end.size() == 1)
        rentry = end[0];
    else {
        string l = "exit";
        rentry = new BasicBlock(l, func);
        for (auto bb : end) {
            bb->succ.push_back(rentry);
            rentry->pred.push_back(bb);
        }
    }
    return rentry;
}


map<Instruction*, bool> marked;

void SccpPass::mark(Function *func) {
    BasicBlock *entry = gen_rcfg(func);
    //DomNode *root = Gen_Dom_Tree(entry);

    set<Instruction*> workList;
    marked.clear();
    for (auto bb : func->block_list) {
        for (auto inst : bb->local_instr) {
            marked[inst] = false;
            if (true) {//inst is critical
                marked[inst] = true;
                workList.insert(inst);
            }
        }
    }

    while (!workList.empty()) {
        auto inst = *(workList.begin());//assume i is x <- y op z
        workList.erase(inst);

        //mark y

        //mark z

        //mark branch
    }
}

void SccpPass::sweep(Function *func) {
    for (auto bb : func->block_list) {
        for (auto inst : bb->local_instr) {
            if (!marked[inst]) {
                if (inst->instType == CONDBR) {
                    //condBr to jump
                }

                if (inst->instType != BR) {
                    DeleteInstruction(inst);
                }
            }
        }
    }
}

void dead(Function *func){

}


void SccpPass::rebuildBlock(Function *func) {
    stack<BasicBlock*> stk;
    unordered_set<BasicBlock*> visited;
    vector<BasicBlock*> new_block_list;

    //find the entry block
    for (auto BB : func->block_list) {
        if (BB->pred.empty()) {
            stk.push(BB);
            break;
        }
    }

    while (!stk.empty()) {
        auto curBB = stk.top();
        stk.pop();

        if (visited.find(curBB) == visited.end()) {
            visited.insert(curBB);
            new_block_list.push_back(curBB);

            for (auto it = curBB->succ.rbegin(); it != curBB->succ.rend(); ++it) {
                stk.push(*it);
            }
        }
    }

    for (auto BB : func->block_list) {
        if (visited.find(BB) == visited.end()) {
            UnlinkBlock(BB, BB->succ[0]);
//            for (auto pred : BB->pred) {
//                for (auto it = pred->succ.begin(); it != pred->succ.end(); ++it) {
//                    if ((*it) == BB) {
//                        pred->succ.erase(it);
//                        break;
//                    }
//                }
//            }
//
//            auto succ = BB->succ[0];
//            for (auto it = succ->pred.begin(); it != succ->pred.end(); ++it) {
//                if ((*it) == BB) {
//                    succ->pred.erase(it);
//                    break;
//                }
//            }
        }
    }
    func->block_list = new_block_list;
}

bool changed;

void SccpPass::clean(Function *func) {
    do {
//        gu->Emit(std::cerr);
        changed = false;
        mergePass(func->block_list);
//        cerr <<"clean in"<<endl;
//        cerr << func->block_list.size() << endl;
//        cerr << "clean out" << endl;
    } while (changed);
}

void SccpPass::mergePass(vector<BasicBlock *>& postorder) {
//    cerr <<"pass in"<<endl;
    for (auto it = postorder.begin(); it != postorder.end(); ++ it) {
        auto bb = *it;
        auto br_inst = bb->local_instr.back();
        if (br_inst->instType == CONDBR) {
            auto condBr = dynamic_cast<CondBrInstruction*>(br_inst);
            if (condBr->trueLabel == condBr->falseLabel) {
                // br i1, bb, bb --> br bb
                IRInstruction* br = new BrInstruction(condBr->trueLabel);
                bb->local_instr.back() = br;
                changed = true;
            }
        }
        if (br_inst->instType == BR) {// i -> j
            auto bb_succ = bb->succ[0];
            // if only has br
            if (bb->pred.size() == 1 && bb->local_instr.size() == 1) { // for entry , this opt is forbidden
                // p -> i(empty) -> j  -->  p -> j
//                for(auto pred: bb->pred){
//                    // upt pred
//                    auto it1 = std::find(pred->succ.begin(),pred->succ.end(),bb_succ);
//                    auto it2 = std::find(pred->succ.begin(),pred->succ.end(),bb);
//                    if(it1 == pred->succ.end()){
//                        *it2 = bb_succ;
//                    }
//                    else{
//                        pred->succ.erase(it2);
//                    }
//
//                    // upt succ
//                    if(std::find(bb_succ->pred.begin(), bb_succ->pred.end(), pred) == bb_succ->pred.end()){
//                        bb_succ->pred.push_back(pred);
//                    }
//                }
//
//                // upt succ
//                auto it3 = std::find(bb_succ->pred.begin(), bb_succ->pred.end(), bb);
//                bb_succ->pred.erase(it3);
//
//                // modify phi
//                for(auto instr:bb_succ->local_instr){
//                    if(instr->instType != PHI) break;
//                    auto phi = dynamic_cast<PhiInstruction*>(instr);
//                    if(!phi->mp.count(bb->pred[0])){
//                        phi->mp[bb->pred[0]] = phi->mp[bb];
//                    }
//                    phi->mp.erase(bb);
//
//                }
//                replaceAllUsesOf(bb,bb_succ);
//                remove_block_from_blocklist(bb,bb->func_belong);
//                if(bb == bb->func_belong->entry){
//                    bb->func_belong->entry = bb_succ;
//                }
//                changed = true;
//                continue;
            }

            if (bb_succ->pred.size() == 1) {
                // i -> j  -->  -> ij ->
                // del br in bb'block;
                assert(bb->local_instr.back()->instType == CONDBR || bb->local_instr.back()->instType == BR);
                bb->local_instr.erase(bb->local_instr.end()-1);

                for(auto instr: bb_succ->local_instr){
                    Insert_instr_beforeBr(instr,bb);
                }

                for(auto succ: bb_succ->succ){
                    auto it1 = std::find(succ->pred.begin(),succ->pred.end(),bb);
                    auto it2 = std::find(succ->pred.begin(),succ->pred.end(),bb_succ);
                    if(it1 == succ->pred.end()){
                        *it2 = bb;
                    }
                    else{
                        succ->pred.erase(it2);
                    }
                }

                bb->succ = bb_succ->succ;
                remove_block_from_blocklist(bb_succ,bb_succ->func_belong);
                replaceAllUsesOf(bb_succ,bb);
                changed = true;
            }

            if (bb_succ->local_instr.size()==1 && bb_succ->local_instr.back()->instType==CONDBR) {
//                changed = true;
            }
        }
    }
//    cerr<<"pass out"<<endl;
}

void SccpPass::mergeBlock(Function *func) {
    assert(func->block_list[0] == func->entry);
    bool merged;
    do {
        merged = false;
        for (auto BB : func->block_list) {
            if (BB->succ.size() == 1 && BB->succ[0]->pred.size() == 1) {

                auto next = BB->succ[0];

                // move instr to succ
                stack<Instruction*> stk;
                for (auto inst : BB->local_instr) {
                    if (inst->instType != BR) {
                        stk.push(inst);
                    }
                    inst->deleted = true;
                }

                while(!stk.empty()){
                    Remove_LocalInstr(stk.top());
                    Insert_instr_atFront(stk.top(),next);
                    stk.pop();
                }

                // link its pred and succ
                for (auto pred : BB->pred) {
                    for (auto succ : pred->succ) {
                        if (succ == BB) {
                            succ = next;
                            break;
                        }
                    }
                }

                BB->succ[0]->pred.clear();
                BB->succ[0]->pred = BB->pred;
//                BB->succ[0]->label = BB->label;

                replace_it(BB, next);
                // finally delete itself func's block_list
                for(auto it = func->block_list.begin(); it != func->block_list.end(); ++it){
                    if((*it) == BB){
                        func->block_list.erase(it);
                        break;
                    }
                }

                merged = true;
                break;
            }
        }
    } while (merged);

}

void SccpPass::renameBlock(Function *func) {
    for (int i = 0; i < func->block_list.size(); ++i) {
        auto bb = func->block_list.at(i);
        bb->label = to_string(i);
    }
}

void SccpPass::mergeBranch(Function *func) {

//    bool merged;
//    do {
//        merged = false;
//        for (auto BB : func->block_list) {
//            stack<Instruction *> stk;
//            for (auto instr: BB->local_instr) {
//                if (instr->instType == BR) {
//                    //move instructions to succ
//                    //if(!BB->succ.empty()) {
//                    while (!stk.empty()) {
//                        BB->succ[0]->insertCodeAtFront(stk.top());
//                        stk.pop();
//                    }
//                    //}
//                    //then del block
//                    //cerr << "del: " << BB->label << endl;
//                    DelBlock(func, BB);
//                    merged = true;
//                    break;
//                } else if (instr->instType == CONDBR) {
//                    auto _instr = (CondBrInstruction *) instr;
//                    if (BB->const_table.find(_instr->condition) != BB->const_table.end()) {
//                        BasicBlock *next;
//                        if (((Bool_Const *) BB->const_table[_instr->condition])->value) {
//                            next = BB->succ[0];
//                        } else {
//                            next = BB->succ[1];
//                        }
//                        //if(!BB->succ.empty()) {
//                        while (!stk.empty()) {
//                            next->insertCodeAtFront(stk.top());
//                            stk.pop();
//                        }
//                        //}
//
//                        //cerr << "del: " << BB->label << endl;
//                        DelBlock(func, BB);
//                        merged = true;
//                        break;
//                    }
//                } else {
//                    stk.push(instr);
//                }
//            }
//        }
//    } while (merged);

}

void SccpPass::run() {
    for (auto &it1: this->gu->func_table) {
        auto func = it1.second;
        if(func->entry == nullptr) continue;

//        func->Emit(std::cerr);
        constProp(func);
//        rebuildBlock(func);
//        constProp(func);
        clean(func);
        constProp(func);
//        rebuildBlock(func);
//        mergeBlock(func);
        deadCodeEli(func);


//        renameBlock(func);
    }
}

void Cvt_CondBr(CondBrInstruction *instr, bool cond) {
    BasicBlock *trueBlock = instr->trueLabel, *falseBlock = instr->falseLabel, *use, *not_use;
    use = cond ? trueBlock : falseBlock;
    not_use = cond ? falseBlock : trueBlock;
    BasicBlock *nowBlock = instr->block;

    // delete condbr instr and replace it with this instr;
    BrInstruction *br = new BrInstruction(use);
    br->block = nowBlock;
    auto &v = nowBlock->local_instr;
    auto it = v.begin();
    for(; it != v.end(); ++it){
        if(*it == instr){
            it = v.erase(it);
            break;
        }
    }
    v.insert(it, br);
    instr->deleted = true;
    DelUseOfInstruction(instr);
    // Unlink nowBlock - not_use
    // TODO: 理论上需要将UnlinkBlock的phi指令加到queue里
    UnlinkBlock(nowBlock, not_use);
}

void replace_it(ValueRef *old, ValueRef *now) {
    for (auto instr: old->use) {
        q.push(instr);
    }
    replaceAllUsesOf(old, now);
}

// use it to remove all connections with refs(def/use), also, remove it from block
void delete_it(Instruction * instr){
    DelUseOfInstruction(instr);
    Remove_LocalInstr(instr);
}
