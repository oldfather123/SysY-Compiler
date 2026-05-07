#pragma once
#include "llvm_ir.h"

namespace llvm_ir {
namespace dce {
    // DCE 优化 pass 主入口
    void runOnModule(Module& module);
    
    // 删除未被调用的函数
    void removeUnusedFunctions(Module& module);
    
    // 删除死指令
    int removeDeadInstructions(Function& function);
} // namespace dce
} // namespace llvm_ir 