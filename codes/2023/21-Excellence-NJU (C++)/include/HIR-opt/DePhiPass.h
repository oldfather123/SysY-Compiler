//
// Created by q1w2e3r4 on 23-8-8.
//

#include "GlobalUnit.h"
#include "HOptUtils.h"
#ifndef COMPILER_DEPHIPASS_H
#define COMPILER_DEPHIPASS_H

class DePhiPass {
public:
    GlobalUnit *gu;

    DePhiPass(GlobalUnit * gu){ this->gu = gu; }

    void run();
};


#endif //COMPILER_DEPHIPASS_H
