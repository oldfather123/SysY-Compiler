#ifndef LLVM_IR_DEAD_LOOP_ELIM_H
#define LLVM_IR_DEAD_LOOP_ELIM_H

#include "llvm_ir.h"

namespace llvm_ir {

class Module;

class DeadLoopElimination {
public:
    DeadLoopElimination(Module& m) : _module(m) {}
    bool run();

private:
    bool runOnFunction(Function& F);
    Module& _module;
};

} // namespace llvm_ir

#endif // LLVM_IR_DEAD_LOOP_ELIM_H 