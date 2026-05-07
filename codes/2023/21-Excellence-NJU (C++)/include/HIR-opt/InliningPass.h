//
// Created by divin on 23-8-20.
//
#include "GlobalUnit.h"

#ifndef COMPILER_INLININGPASS_H
#define COMPILER_INLININGPASS_H

using namespace std;

class InliningPass {
public:
    GlobalUnit *gu;

    InliningPass(GlobalUnit *gu) { this->gu = gu;}

    void Build_Simple_Call_Graph();

    void Build_Call_Graph();

    bool has_se_inst(Function* func);

    void Set_Side_Effect();

    void debug(map<string,Function*>& func_table);

    void run();
};


#endif //COMPILER_INLININGPASS_H
