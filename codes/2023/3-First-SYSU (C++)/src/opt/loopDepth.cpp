#include "loopDepth.h"

void getLoopDepth(BlockPtr block){
    int depth = 0;
    for(int i=0;i<block->basicBlocks.size();i++){
        auto name = block->basicBlocks[i]->label->name;
        if(name.find("while.cond")!=name.npos){
            depth++;
        }
        else if(name.find("while.end")!=name.npos){
            depth--;
        }
        block->basicBlocks[i]->loopDepth = depth;
        cerr<<name<<"   "<<depth<<endl;
    }
}