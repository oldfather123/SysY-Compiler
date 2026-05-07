#ifndef LLVM_IR_PHIELIMINATION_H
#define LLVM_IR_PHIELIMINATION_H

#include "llvm_ir.h"

namespace llvm_ir {
namespace phi_elimination {

// 主入口：消除所有PHI指令，返回是否有修改
bool DemoteAllPHIs(Module& M);

} // namespace phi_elimination
} // namespace llvm_ir

#endif // LLVM_IR_PHIELIMINATION_H 