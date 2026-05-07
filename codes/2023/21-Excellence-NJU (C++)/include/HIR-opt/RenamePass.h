//
// Created by q1w2e3r4 on 23-8-7.
//

#ifndef COMPILER_RENAMEPASS_H
#define COMPILER_RENAMEPASS_H


#include "GlobalUnit.h"

class RenamePass {
public:
    GlobalUnit* gu;

    void run();

    void Rename(Function* func);

    RenamePass(GlobalUnit * gu) { this->gu = gu; }
};


#endif //COMPILER_RENAMEPASS_H
