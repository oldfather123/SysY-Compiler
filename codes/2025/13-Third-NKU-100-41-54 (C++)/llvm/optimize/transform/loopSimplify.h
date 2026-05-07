#ifndef LOOPSIMPLIFY_H
#define LOOPSIMPLIFY_H
#include "../../include/ir.h"
#include "../pass.h"

#include "../analysis/dominator_tree.h"

class LoopSimplifyPass : public IRPass { 
private:

public:
    LoopSimplifyPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif