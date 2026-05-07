#ifndef CFG_SIMPLIFY_H
#define CFG_SIMPLIFY_H

#include "llvm_ir.h"

namespace llvm_ir {
namespace cfg {

// 空跳转块消除优化
void RemoveTrampolineBlocks(Function& F);

void MakeFunctionOneExit(Function& F); // 确保函数只有一个返回块

} // namespace cfg
} // namespace llvm_ir

#endif // CFG_SIMPLIFY_H 