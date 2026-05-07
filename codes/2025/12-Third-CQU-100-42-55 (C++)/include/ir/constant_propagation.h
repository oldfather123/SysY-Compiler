#pragma once
#include "llvm_ir.h"
#include <unordered_map>
#include <set>
#include <vector>

namespace ir_opt {
    // 稀疏条件常量传播 (SCCP)
    bool sparseConditionalConstantPropagation(llvm_ir::Module& module);
} // namespace ir_opt 