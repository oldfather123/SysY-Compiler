#ifndef LOOPSTRENGTHREDUCE_H
#define LOOPSTRENGTHREDUCE_H
#include "../../include/ir.h"
#include "../pass.h"
#include "ScalarEvolution.h"

class LoopStrengthReducePass : public IRPass { 
private:
    void LoopStrengthReduce(CFG* cfg);
public:
    LoopStrengthReducePass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
    void GepStrengthReduce();
};

#endif
