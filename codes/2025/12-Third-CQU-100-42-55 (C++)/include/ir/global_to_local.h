#ifndef LLVM_IR_GLOBAL_TO_LOCAL_H
#define LLVM_IR_GLOBAL_TO_LOCAL_H

#include "llvm_ir.h"

namespace llvm_ir {
namespace global_to_local {

// 如果全局变量只在一个函数中被使用，将其降级为该函数入口处的局部变量（alloca）。
// 目前仅处理标量（非数组）全局变量，并在入口对其进行零初始化。
// 返回是否进行了任何修改。
bool runOnModule(Module& M);

} // namespace global_to_local
} // namespace llvm_ir

#endif // LLVM_IR_GLOBAL_TO_LOCAL_H 