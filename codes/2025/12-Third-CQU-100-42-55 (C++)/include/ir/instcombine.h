#ifndef INSTCOMBINE_H
#define INSTCOMBINE_H
#include "llvm_ir.h"

namespace llvm_ir {
namespace instcombine {
    // instcombine 优化 pass 主入口
    bool runOnModule(Module& module);

    bool runGlobalConstProp(Module& M);
} // namespace instcombine
} // namespace llvm_ir 
#endif