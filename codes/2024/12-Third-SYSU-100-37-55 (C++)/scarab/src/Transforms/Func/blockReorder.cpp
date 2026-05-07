#include "blockReorder.h"

bool isDeadBlock(BasicBlockPtr bb){
    if(bb->instructions.empty() || bb->predBasicBlocks.empty()){
        return true;
    }
    return false;
}

void deleteDeadBlock(FunctionPtr& func){
    if(func->isLibraryFunction)
        return;
    for(auto it = func->basicBlocks.begin() + 1; it != func->basicBlocks.end(); it++){
        if(isDeadBlock(*it)){
            func->basicBlocks.erase(it);
        }
    }
    return;
}

int computeDistance(map<BasicBlockPtr, int>pos, vector<BasicBlockPtr>bbs){
    int ret = 0;
    for(auto bb : bbs){
        for(auto succ : bb->succBasicBlocks){
            ret += abs(pos[bb] - pos[succ]);
        }
    }
    return ret;
}

void sortBb(map<BasicBlockPtr, int>& pos, vector<BasicBlockPtr>& bbs){
    bool changed = false;
    int origin = computeDistance(pos, bbs);
    if(bbs.size() < 10)
        return;
    for(int i = 1; i < bbs.size() - 1; i++){
        for(int j = i + 1; j < bbs.size(); j++){
            int temp = pos[bbs[i]];
            pos[bbs[i]] = pos[bbs[j]];
            pos[bbs[j]] = temp;
            int afterChange = computeDistance(pos, bbs);

            if(afterChange < origin){
                origin = afterChange;
            }else{
                pos[bbs[j]] = pos[bbs[i]];
                pos[bbs[i]] = temp;
            }
        }
    }
    sort(bbs.begin(), bbs.end(), [&](const BasicBlockPtr& a, const BasicBlockPtr& b) {return pos[a] < pos[b];});
}

void blockReorder(Module& ir){
    for(auto func : ir.globalFunctions){
        if(!func->isLibraryFunction){
            map<BasicBlockPtr, int>bbPos;
            int count = 0;
            for(auto bb : func->basicBlocks){
                bbPos[bb] = ++count;
            }

            sortBb(bbPos, func->basicBlocks);
        }
    }
}