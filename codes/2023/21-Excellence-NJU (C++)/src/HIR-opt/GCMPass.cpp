//
// Created by q1w2e3r4 on 23-8-17.
//

#include "HIR-opt/GCMPass.h"
#include "HIR-opt/LoopPass.h"
#include "HIR-opt/DomTreePass.h"
#include <map>

map<ValueRef*, BasicBlock*> early_block;
map<ValueRef*, BasicBlock*> late_block;
map<ValueRef*, BasicBlock*> best_block;
set<ValueRef*> placed1;
vector<BasicBlock*> order;
set<Instruction*> vis2;

void Place_it(ValueRef* ref);

void PostOrder1(DomNode * entry){
    for(auto child: entry->child){
        PostOrder1(child);
    }
    order.push_back(entry->bb);
}

bool Pinned(Instruction * instr){
    switch(instr->instType){
        case PHI:
        case BR:
        case CONDBR:
        case RET:
        case CALL:
        case LOAD:
        case STORE:
            return true;
        default:
            return false;
    }
}

bool IsConst(ValueRef * ref){
    return (ref->type == IntConst || ref->type == FloatConst || ref->type == BoolConst);
}

BasicBlock * Find_LCA(BasicBlock* a, BasicBlock* b);

void GCMPass::run() {
    DomTreePass * domTreePass = new DomTreePass(gu);
    domTreePass->run(); // step1: get dom tree

    LoopPass * loopPass = new LoopPass(gu);
    loopPass->analysis(); // step2: get all loops and depth

    for(auto&[name,func] :gu->func_table){
        if(func->entry == nullptr) continue;
        early_block.clear(); late_block.clear(); vis2.clear(); order.clear();
        PostOrder1(func->entry->domNode);
        // step3 schedule_early
        //cerr << "early block:" << endl;
        for(auto block: order){
            for(auto instr: block->local_instr){
                if(Pinned(instr)){
                    vis2.insert(instr);
                    if(!instr->def_list.empty()){
                        auto def = *(instr->def_list.begin());
                        early_block[def] = block;
                    }
                    for(auto use: instr->use_list){
                        if(use->type == SYMBOL || use->type == Block || IsConst(use)) continue; // these don't need to move
                        if(use->def.empty()) continue; // func params don't have def
                        Schedule_early(use->def[0],func);
                    }
                }
            }
        }

        // step4 schedule_late
        //cerr << "late block:" << endl;
        vis2.clear();
        for(auto block: order){
            for(auto instr: block->local_instr){
                //instr->outPut(std::cerr);
                if(Pinned(instr)){
                    vis2.insert(instr);
                    if(!instr->def_list.empty()) {
                        auto def = *(instr->def_list.begin());
                        for(auto use_instr: def->use){
                            if(Pinned(use_instr)) continue;
                            Schedule_late(use_instr,func);
                        }
                    }
                }
            }
        }
//        cerr << "early block" << endl;
//        for(auto &[ref,block]: early_block){
//            cerr << ref->name << " " << block->label << endl;
//        }
//
//        cerr << "late block" << endl;
//        for(auto &[ref,block]: late_block){
//            cerr << ref->name << " " << block->label << endl;
//        }

        // step5 place instr in better pos;
        vis2.clear();
        for(auto&[early,block]: early_block){
            if(early->get_def() == nullptr)
                continue;
            else if(Pinned(early->get_def()))
                continue;
            Place_instr(early);
        }

        // step6? really place it;
        for(auto to_place: best_block){
            Place_it(to_place.first);
        }
    }
}

void GCMPass::Schedule_early(Instruction* instr,Function* func) {
    if(vis2.count(instr)) return;
    vis2.insert(instr);

    BasicBlock* early = func->entry;
    for(auto use: instr->use_list){
        if(use->type == SYMBOL || use->type == Block || IsConst(use)) continue; // these don't need to move
        //TODO: SYMBOL 可以考虑挪一下？
        if(use->def.empty()) continue; // func params don't have def
        if(Pinned(use->def[0])){ // pinned instr, its early_block is itself
            early_block[use] = dynamic_cast<IRInstruction*>(use->def[0])->block;
            //cerr << use->name << " : " << early_block[use]->label << endl;
        }
        else {
            Schedule_early(use->def[0], func);
        }
        BasicBlock * scheduled = early_block[use];
        if(early->domNode->depth < scheduled->domNode->depth){
            early = scheduled;
        }
    }

    auto def = *(instr->def_list.begin());
    early_block[def] = early;
    //cerr << def->name << " : " << early->label << endl;
}

void GCMPass::Schedule_late(Instruction* instr,Function* func) {
//    instr->outPut(std::cerr);
    if(vis2.count(instr)) return;
    vis2.insert(instr);

    ValueRef* ref = *(instr->def_list.begin()); //(def) -> schedule_late this
    //cerr << "!" << ref->name << endl;
    BasicBlock * lca = nullptr;

    for(auto use_instr:ref->use){
        ValueRef * use_ref = *(use_instr->def_list.begin());
        BasicBlock * ans = nullptr;
        if (use_instr->instType == PHI) { // phi instr judge specially
            auto phi = dynamic_cast<PhiInstruction *>(use_instr);
            for (auto &[block, value]: phi->mp) {
                if (value == ref) {
                    ans = block;
                    break;
                }
            }
        }
        else if(Pinned(use_instr)) { // BR, CONDBR, RET...
            ans = dynamic_cast<IRInstruction*>(use_instr)->block;
        }
        else {
            Schedule_late(use_instr, func);
            ans = late_block[use_ref];
        }
        lca = Find_LCA(lca,ans);
    }

    late_block[ref] = lca;
    //cerr << ref->name << " : " << lca->label << endl;
}

void GCMPass::Place_instr(ValueRef *ref) {
//    cerr << "!" << ref->name << endl;
    BasicBlock* best = late_block[ref] == nullptr? dynamic_cast<IRInstruction*>(ref->get_def())->block : late_block[ref];
    BasicBlock* earliest = early_block[ref];
    BasicBlock* now = best;
    while(now != earliest){
        if(now->loop_depth < best->loop_depth){
            best = now;
        }
        now = now->domNode->IDOM->bb;
    }
    best_block[ref] = best;
}

void Place_it(ValueRef* ref){
    if(placed1.count(ref)) return;
    placed1.insert(ref);

    auto def_instr = dynamic_cast<IRInstruction*>(ref->get_def());
    //if(best_block[ref] == def_instr->block) return;
    for(auto before: def_instr->use_list){
        if(best_block.count(before) && !placed1.count(before)){
            Place_it(before);
        }
    }
    Remove_LocalInstr(def_instr);

    BasicBlock* bb = def_instr->block;
    auto late_it = bb->local_instr.end();
    for(auto it = bb->local_instr.begin(); it != bb->local_instr.end(); ++it){
        if((*it)->instType == PHI) continue;
        if((*it)->use_list.count(ref)){
            late_it = it; break;
        }
    }

    if(late_it != bb->local_instr.end()){
        bb->local_instr.insert(late_it,def_instr);
    }
    else{
        Insert_instr_beforeBr(def_instr,bb);
    }
}

BasicBlock * Find_LCA(BasicBlock* a, BasicBlock* b){
    if(a == nullptr) return b;
//    cerr << a->label << " " << b->label << endl;
    DomNode *d1 = a->domNode, *d2 = b->domNode;
    while(d1->depth < d2->depth){
        d2 = d2->IDOM;
    }
    while(d1->depth > d2->depth){
        d1 = d1->IDOM;
    }
    while(d1 != d2){
        d1 = d1->IDOM;
        d2 = d2->IDOM;
    }
    return d1->bb;
}

