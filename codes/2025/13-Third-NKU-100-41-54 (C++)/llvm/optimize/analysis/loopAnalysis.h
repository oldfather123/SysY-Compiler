#ifndef LOOPANALYSIS_H
#define LOOPANALYSIS_H
#include "../../include/ir.h"
#include "../pass.h"

#include "../analysis/dominator_tree.h"

class LoopAnalysisPass : public IRPass { 
private:

public:
    LoopAnalysisPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif