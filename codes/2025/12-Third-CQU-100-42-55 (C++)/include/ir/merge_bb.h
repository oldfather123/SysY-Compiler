
#ifndef MERGE_BB_H
#define MERGE_BB_H

#include "llvm_ir.h"

namespace llvm_ir {
namespace merge_bb {

// 基本块合并优化的模块级入口
bool runOnModule(Module& M);

// 基本块合并优化
// 合并满足条件的基本块：
// 1. 基本块A只有一个后继B
// 2. 基本块B只有一个前驱A
// 3. 基本块A的终止指令是无条件跳转
// 4. 基本块A和B都必须在included中（如果included非空）
// 5. 基本块A和B都不能在excluded中
void MergeBasicBlocks(Function& F, std::vector<BasicBlock*> &included, std::vector<BasicBlock*> &excluded);

// 重载版本：向后兼容，没有included/excluded限制
void MergeBasicBlocks(Function& F);

// 辅助函数：合并两个基本块
void mergeBlocks(BasicBlock* bbA, BasicBlock* bbB, Function& F);

}  // namespace merge_bb
}  // namespace llvm_ir

#endif  // MERGE_BB_H
