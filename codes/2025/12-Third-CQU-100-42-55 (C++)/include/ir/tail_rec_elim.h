// Tail recursion elimination pass
#pragma once
#include "llvm_ir.h"
#include "cfg.h"

namespace llvm_ir {
namespace tail_rec_elim {

// Run tail recursion elimination on every function in the module.
// Detects simple self tail calls (last non-terminator instruction immediately
// followed by a return that returns the call's value, or a void return) and
// converts them into a loop by:
// 1. Inserting a new header basic block before the original entry (once per function)
// 2. Creating PHI nodes in the original entry for each formal parameter
// 3. Replacing parameter uses with the PHI nodes
// 4. For every tail recursive call site, removing the call+return and adding
//    an incoming edge with arguments to each PHI plus a branch to the original entry
// The transformation is repeated until no further tail recursive sites remain.
void runOnModule(Module& M);

} // namespace tail_rec_elim
} // namespace llvm_ir
