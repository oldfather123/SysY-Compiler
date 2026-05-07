//
// Created by q1w2e3r4 on 23-7-9.
//

#include "Builder.h"
using namespace std;

BasicBlock * Builder::AppendBasicBlock(Function *func, std::string& blockName) {
    BasicBlock * block = new BasicBlock(blockName,func);
    func->block_list.push_back(block);
    return block;
}

void Builder::PositionBuilderAtEnd(BasicBlock *block) {
    this->nowBlock = block;
}

void Builder::AppendCode(IRInstruction* instruction) {
    if(this->nowBlock == nullptr){ // 说明在全局块
        this->globalUnit->addInstr(instruction);
    }
    else{ //否则加到对应块内
        if(instruction->instType == ALLOCA){
            this->curFunc->entry->appendCode(instruction);
        }
        else {
            this->nowBlock->appendCode(instruction);
        }
    }
    instruction->block = nowBlock;
}

void Builder::LinkBlock(BasicBlock *succ) {
    // nowBlock ----> succ;
    this->nowBlock->succ.push_back(succ);
    succ->pred.push_back(this->nowBlock);
}


