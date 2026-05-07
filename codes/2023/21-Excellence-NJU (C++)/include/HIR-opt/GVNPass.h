//
// Created by q1w2e3r4 on 23-8-16.
//

#include "GlobalUnit.h"
#include "HOptUtils.h"
#include "IRInstruction.h"
#include "SccpPass.h"
#ifndef COMPILER_GVNPASS_H
#define COMPILER_GVNPASS_H

enum UnaryType{zext,xori,fneg,itfp,fpti};
enum BinaryType{add,sub,mul,divide,mod,andi,ori,eq,ne,lt,le,gt,ge,sll,sra,gep};

class GVNPass {
public:
    GlobalUnit * gu;

    GVNPass(GlobalUnit * gu){ this->gu = gu; }

    void run();

    void GVN(Function* func);
};


#endif //COMPILER_GVNPASS_H
