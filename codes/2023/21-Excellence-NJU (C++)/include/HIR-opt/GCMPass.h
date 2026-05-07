//
// Created by q1w2e3r4 on 23-8-17.
//

#include "GlobalUnit.h"
#ifndef COMPILER_GCMPASS_H
#define COMPILER_GCMPASS_H

class GCMPass {
public:
    GlobalUnit * gu;

    GCMPass(GlobalUnit * gu){ this->gu = gu; }

    void run();

    void Schedule_early(Instruction* instr,Function* func);

    void Schedule_late(Instruction* instr,Function* func);

    void Place_instr(ValueRef* ref);
};


#endif //COMPILER_GCMPASS_H
