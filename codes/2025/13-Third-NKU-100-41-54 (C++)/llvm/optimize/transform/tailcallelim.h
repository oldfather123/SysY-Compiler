#ifndef TAILCALLELIM_H
#define TAILCALLELIM_H
#include "../../include/ir.h"
#include "../pass.h"

class TailCallElimPass : public IRPass { 
private:
	
public:
	bool IsTailCallFunc(CFG *C);
    TailCallElimPass(LLVMIR *IR) : IRPass(IR) {}
    void TailCallElim(CFG *C);
    void Execute();
};

#endif