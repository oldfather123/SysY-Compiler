//
// Created by q1w2e3r4 on 23-8-13.
//

#include "HIR-opt/LoopPass.h"
#include <queue>
#include <vector>

vector<pair<BasicBlock*, BasicBlock*>> backedge; // <header,latch>
map<BasicBlock*,Loop*> belong_loop;
vector<Loop*> loop_info;

void Mark_depth(Function* func);
void mark(Loop * loop);

void PostOrder(DomNode * dom){
    for(auto node: dom->child){
        PostOrder(node);
    }

    for(auto pred: dom->bb->pred){
        if(pred->domNode->nodeId > dom->nodeId){
            // find a ledge
            backedge.emplace_back(dom->bb,pred);
            //cerr << "Backedge: " << pred->label << " -> " << dom->bb->label << endl;
        }
    }


}

void LoopPass::analysis() {
    for(auto& [name,func]: gu->func_table){
        if(func->entry != nullptr) {
            backedge.clear();
            belong_loop.clear();
            loop_info.clear();
            FindLoop(func);
            Mark_depth(func);
        }
    }
}

void LoopPass::FindLoop(Function *func) {
    // step 1,find latchs
    PostOrder(func->entry->domNode);

    // step 2,find loop
    // one BasicBlock should have only one loop, if not ,merge them
    for(int i=0;i<backedge.size();++i){
        BasicBlock * header = backedge[i].first;
        Loop * loop = new Loop();
        loop->header = header;

        queue<BasicBlock *> q;
        q.push(backedge[i].second);
        loop->latch.push_back(backedge[i].second);
        while(i+1<backedge.size() && backedge[i+1].first == header){
            q.push(backedge[i+1].second);
            loop->latch.push_back(backedge[i+1].second);
            ++i;
        }

        while(!q.empty()){
            BasicBlock * now = q.front(); q.pop();
            Loop* subloop = nullptr;
            auto it = belong_loop.find(now);
            if(it != belong_loop.end()) subloop = it->second;

            if(subloop != nullptr){
                loop->add_loop(subloop);
                subloop->fa_loop = loop;
                assert(subloop->header != header);
                for(auto pred: subloop->header->pred){
                    if(!subloop->contains(pred))
                        q.push(pred);
                }
            }
            else{
                loop->add_block(now);
                if(now != header) {
                    for (auto pred: now->pred) {
                        if(!loop->contains(pred))
                            q.push(pred);
                    }
                }
            }
        }

        for(auto block: loop->contain_blocks){
            belong_loop[block] = loop;
        }
        //loop->debug();
        loop_info.push_back(loop);
    }


}

void LoopPass::Lcssa() {
    for(auto& [name,func]: gu->func_table){
        if(func->entry != nullptr) {
            backedge.clear();
            belong_loop.clear();
            loop_info.clear();
            FindLoop(func);
            BuildLoop(func);
        }
    }
}

void LoopPass::BuildLoop(Function* func) {
    // change it to LCSSA
    // now we only add preheader to each loop
    // TODO: num of latchs -> 1;
    // TODO: dedicated exit
    for(auto loop: loop_info){
        string name = "preheader";
        BasicBlock* preheader = new BasicBlock(name,func);
        for(auto prev: loop->header->pred){
            if(!loop->contains(prev))
                Insert_Block(prev,loop->header,preheader);
        }
        auto& v = func->block_list;
        for(auto it = v.begin(); it != v.end(); ++it){
            if(*it == loop->header){
                v.insert(it,preheader);
                break;
            }
        }
    }
}

void Mark_depth(Function* func){
    for(auto block:func->block_list){ // init;
        block->loop_depth = 0;
        block->belong_loop = nullptr;
    }
    for(auto loop:loop_info){
        if(loop->fa_loop == nullptr){
            loop->depth = 1;
            mark(loop);
        }
    }
}

void mark(Loop * loop){
    for(auto block: loop->contain_blocks){
        block->belong_loop = loop;
        block->loop_depth = loop->depth;
        //cerr << block->label << " " << block->loop_depth << endl;
    }

    for(auto subloop: loop->contain_loops){
        subloop->depth = loop->depth + 1;
        mark(subloop);
    }
}

