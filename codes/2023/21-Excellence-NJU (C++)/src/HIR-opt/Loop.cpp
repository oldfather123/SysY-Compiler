//
// Created by q1w2e3r4 on 23-8-13.
//

#include "HIR-opt/Loop.h"

//Loop::Loop(vector<BasicBlock*>& start, BasicBlock* end) {
//    this->header = end;
//    this->latch = start;
//    this->fa_loop = nullptr;
//    this->preheader = nullptr;
//}
void Loop::debug() {
    cerr << "Loop(" << header->label << "):" << endl;
    cerr << "contain_blocks: ";
    for(auto block:contain_blocks){
        cerr << block->label << " ";
    }
    cerr << endl << "contain_loops: ";
    for(auto loop:contain_loops){
        cerr << "Loop(" << loop->header->label << ") ";
    }
    cerr << endl;
}

bool Loop::contains(BasicBlock *block) {
    for(auto loop: contain_loops){
        if(loop->contains(block)){
            return true;
        }
    }
    return contain_blocks.count(block);
}
