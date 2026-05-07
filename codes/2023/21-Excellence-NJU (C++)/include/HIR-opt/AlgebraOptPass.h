//
// Created by q1w2e3r4 on 23-8-14.
//

#include "GlobalUnit.h"
#include "IRInstruction.h"
#include "SccpPass.h"
#ifndef COMPILER_STRENREDUCTIONPASS_H
#define COMPILER_STRENREDUCTIONPASS_H


class AlgebraOptPass {
public:
    GlobalUnit * gu;

    explicit AlgebraOptPass(GlobalUnit* gu){ this->gu = gu;}

    void run();

    void Reduce(Function* func);
};


#endif //COMPILER_STRENREDUCTIONPASS_H
