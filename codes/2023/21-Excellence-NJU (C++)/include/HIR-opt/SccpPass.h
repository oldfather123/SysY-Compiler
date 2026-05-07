//
// Created by divin on 23-8-5.
//
#include "GlobalUnit.h"

#ifndef COMPILER_SCCPPASS_H
#define COMPILER_SCCPPASS_H

using namespace std;

class SccpPass {
public:
    GlobalUnit* gu;

    SccpPass(GlobalUnit* gu){ this->gu = gu;}

    void instrProp(Instruction* instr);

    void constProp(Function* func);

    void calc(Function* func);// no use

    void deadCodeEli(Function* func);// no use

    void rebuildBlock(Function* func);

    BasicBlock* gen_rcfg(Function *func);

    void mark(Function *func);

    void sweep(Function *func);

    void dead(Function *func);

    void clean(Function* func);

    void mergePass(vector<BasicBlock*>& postorder);

    void mergeBlock(Function* func);// no use

    void renameBlock(Function* func);// no use

    void mergeBranch(Function* func);// no use

    void run();

};

#endif //COMPILER_SCCPPASS_H
