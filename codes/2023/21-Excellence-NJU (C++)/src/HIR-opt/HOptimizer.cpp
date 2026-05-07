//
// Created by q1w2e3r4 on 23-7-25.
//

#include <queue>
#include "HIR-opt/HOptimizer.h"
#include "HIR-opt/DomTreePass.h"
#include "HIR-opt/LiveVariableAnalysis.h"
#include "HIR-opt/Mem2RegPass.h"
#include "HIR-opt/SccpPass.h"
#include "HIR-opt/RenamePass.h"
#include "HIR-opt/DePhiPass.h"
#include "HIR-opt/LoopPass.h"
#include "HIR-opt/AlgebraOptPass.h"
#include "HIR-opt/GVNPass.h"
#include "HIR-opt/GCMPass.h"
#include "HIR-opt/InliningPass.h"

HOptimizer::HOptimizer(GlobalUnit *gu) {
    this->globalUnit = gu;
}

void HOptimizer::Optimize() {
    // Dom Tree & DF
    BuildCFG();
    Constlize();

    //globalUnit->Emit(std::cerr);
    DomTreePass* domTreePass = new DomTreePass(this->globalUnit);
    domTreePass->run();

    //LVA
    LiveVariableAnalysis* lva = new LiveVariableAnalysis(this->globalUnit);

    lva->analysis();
//
//     Mem2reg
    Mem2RegPass* mem2reg = new Mem2RegPass(this->globalUnit);
    mem2reg->run();

//  Loop opt
    LoopPass * loopPass = new LoopPass(this->globalUnit);
    loopPass->Lcssa();

    InliningPass *inliningPass = new InliningPass(this->globalUnit);
    inliningPass->run();

//    globalUnit->Emit(std::cerr);
//    debug();
//  SccpPass
//    globalUnit->Emit(std::cerr);
    SccpPass* sccp = new SccpPass(this->globalUnit);
    sccp->run();
//    globalUnit->Emit(std::cerr);
    BuildCFG();

    AlgebraOptPass* algebraOptPass = new AlgebraOptPass(this->globalUnit);
    algebraOptPass->run();
//    globalUnit->Emit(std::cerr);
    GVNPass* gvnPass = new GVNPass(this->globalUnit);
    gvnPass->run();
//    globalUnit->Emit(std::cerr);
    GCMPass* gcmPass = new GCMPass(this->globalUnit);
    gcmPass->run();

    BuildCFG();
//    globalUnit->Emit(std::cerr);
    DePhiPass* dePhiPass = new DePhiPass(this->globalUnit);
   dePhiPass->run();

    // Rename
    RenamePass* renamePass = new RenamePass(this->globalUnit);
    renamePass->run();
    //cerr << "after opt:" << endl;
}

void HOptimizer::BuildCFG() {
    // 1.bfs && remove unvisited blocks
    for(auto&[name,func]: globalUnit->func_table){
        if(func->entry == nullptr) continue;

        set<BasicBlock*> vis;
        queue<BasicBlock*> q;
        q.push(func->entry);
        while(!q.empty()){
            BasicBlock * block = q.front(); q.pop();
            if(vis.count(block)) continue;
            vis.insert(block);
            for(auto bb:block->succ){
                q.push(bb);
            }
        }

        auto& v = func->block_list;
        vector<BasicBlock *> not_visited;
        for(auto & it : v){
            if(!vis.count(it)){ // not visited
                not_visited.push_back(it);
            }
        }

        for(auto bb:not_visited){
            DelBlock(bb);
        }
    }
}

void HOptimizer::debug() {
    for(auto&[name,func]:globalUnit->func_table){
        for(auto block: func->block_list){
            for(auto instr: block->local_instr){
                if(!instr->def_list.empty()){
                    ValueRef * def = *(instr->def_list.begin());
                    cerr << def->name << " :" << endl;
                    for(auto use : def->use){
                        use->outPut(std::cerr);
                    }
                }
            }
        }
    }
}

void HOptimizer::Constlize() {
    for(auto &[name,symbol]: globalUnit->global_symbol_table){
        if(symbol->symbolType->type != ARRAYTYPE && symbol->def.empty()){
            for(auto load: symbol->use){
                ValueRef* val = *(load->def_list.begin());
                replaceAllUsesOf(val,symbol->constVal);
            }
        }
    }
}
