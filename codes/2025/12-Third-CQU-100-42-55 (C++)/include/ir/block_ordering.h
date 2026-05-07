#ifndef BLOCK_ORDERING_H
#define BLOCK_ORDERING_H

#include "llvm_ir.h"
#include "cfg.h"

namespace llvm_ir {

// 基本块排序优化：基于支配树顺序重新排列基本块
// 每个基本块都在所有支配它的基本块之后
void OrderBlocksByDominatorTree(Function& F);

// 辅助函数：获取基本块在支配树中的深度优先遍历顺序
std::vector<BasicBlock*> GetDominatorTreeOrder(Function& F);

} // namespace llvm_ir

#endif // BLOCK_ORDERING_H 