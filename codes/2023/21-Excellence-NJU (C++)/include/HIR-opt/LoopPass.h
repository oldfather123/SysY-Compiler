//
// Created by q1w2e3r4 on 23-8-13.
//

#include "GlobalUnit.h"
#include "Loop.h"
#include "HIR-opt/HOptUtils.h"
#include "HIR-opt/DomTreePass.h"
#ifndef COMPILER_LOOPPASS_H
#define COMPILER_LOOPPASS_H


class LoopPass {
public:
    GlobalUnit * gu;

    explicit LoopPass(GlobalUnit* gu){ this->gu = gu; }

    // analysis loop depth
    void analysis();

    // modify block to lcssa form
    void Lcssa();

    void BuildLoop(Function * func);

    void FindLoop(Function * func);

    void CodeMotion(Function * func);

    //用于代码外提
    void moveToAnotherBlock(IRInstruction* instr, BasicBlock* from, BasicBlock* to);
};


#endif //COMPILER_LOOPPASS_H
