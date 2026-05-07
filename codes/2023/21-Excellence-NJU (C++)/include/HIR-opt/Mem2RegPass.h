//
// Created by q1w2e3r4 on 23-7-27.
//
#include "GlobalUnit.h"
#ifndef COMPILER_MEM2REGPASS_H
#define COMPILER_MEM2REGPASS_H


class Mem2RegPass {
public:
    GlobalUnit * globalUnit;

    explicit Mem2RegPass(GlobalUnit * gu){ this->globalUnit = gu; }

    void run();

    void insertPhi(Function * func);

    void renamePhi(DomNode * root);

    void mergeEntry(Function * func);
};


#endif //COMPILER_MEM2REGPASS_H
