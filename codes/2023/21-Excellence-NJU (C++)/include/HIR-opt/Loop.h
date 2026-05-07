//
// Created by q1w2e3r4 on 23-8-13.
//

#include <vector>
#include "BasicBlock.h"
#include "DomNode.h"
using namespace std;

#ifndef COMPILER_LOOP_H
#define COMPILER_LOOP_H


class Loop {
public:
    Loop* fa_loop = nullptr;
    set<BasicBlock*> contain_blocks;
    set<Loop*> contain_loops;

    BasicBlock * preheader;
    BasicBlock * header;
    vector<BasicBlock *> latch;

    int depth;

    //Loop(vector<BasicBlock*>& start, BasicBlock* end);

    void add_block(BasicBlock * block) { contain_blocks.insert(block); }
    void add_loop(Loop * loop){ contain_loops.insert(loop); }
    bool contains(BasicBlock * block);
    void debug();
};
#endif //COMPILER_LOOP_H
