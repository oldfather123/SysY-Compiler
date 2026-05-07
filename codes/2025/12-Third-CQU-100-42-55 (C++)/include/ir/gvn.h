#pragma once
#include "llvm_ir.h"
#include "function_analysis.h"
#include <unordered_map>
#include <vector>
#include <tuple>

namespace llvm_ir {
namespace gvn {

bool runOnModule(Module& M);

} // namespace gvn
} // namespace llvm_ir