//
// Created by divin on 23-7-27.
//
#include "GlobalUnit.h"


#ifndef COMPILER_LIVEVARIABLEANALYSIS_H
#define COMPILER_LIVEVARIABLEANALYSIS_H

using namespace std;

class LiveVariableAnalysis {
public:
    //
    GlobalUnit* globalUnit;

    LiveVariableAnalysis(GlobalUnit* gu);

    void calcUseDef(Function* func);

    void calcIn(BasicBlock* BB);

    void calcOut(BasicBlock* BB);

    void analysis();

    void printBBIn(BasicBlock* BB);
    void printBBOut(BasicBlock* BB);
    void listDefUse();
    void listInOut();
};


#endif //COMPILER_LIVEVARIABLEANALYSIS_H
