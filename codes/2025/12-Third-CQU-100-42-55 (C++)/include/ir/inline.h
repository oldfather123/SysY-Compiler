#ifndef LLVM_IR_INLINE_H
#define LLVM_IR_INLINE_H

#include "llvm_ir.h"

namespace llvm_ir {

// Forward declaration of Module
class Module;

namespace inliner {

/**
 * @brief Runs the function inlining pass on an entire module.
 * 
 * Iterates through all functions in the module and attempts to inline call sites.
 *
 * @param M The module to process.
 * @return true if the module was modified, false otherwise.
 */
bool runOnModule(Module& M);

void fixupCFGAndPHIs(Function& F);

} // namespace inliner
} // namespace llvm_ir

#endif // LLVM_IR_INLINE_H

