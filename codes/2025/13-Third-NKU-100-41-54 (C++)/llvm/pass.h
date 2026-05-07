#ifndef PASS_H
#define PASS_H
#include "../include/ir.h"
#include <cstdint>
#include <unordered_set>

class IRPass {
protected:
    LLVMIR *llvmIR;

public:
    virtual void Execute() = 0;
    IRPass(LLVMIR *IR) { llvmIR = IR; }
};

#endif