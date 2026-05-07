#ifndef INSTCOMBINE_H
#define INSTCOMBINE_H
#include "../../include/ir.h"
#include "../pass.h"

class InstCombinePass : public IRPass { 
private:
    void ApplyRules();
    void AlgebraSimplify1(LLVMBlock block); // a-(a+b)+b  --> 0
    void AlgebraSimplify2(LLVMBlock block); // (0-a)+b    --> b-a
public:
    InstCombinePass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();

};

#endif