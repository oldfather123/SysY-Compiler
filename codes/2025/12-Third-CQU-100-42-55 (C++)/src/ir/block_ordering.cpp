#include "../../include/ir/block_ordering.h"
#include "../../include/ir/cfg.h"
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <queue>

namespace llvm_ir {

// 深度优先遍历支配树，获取基本块的顺序
void dfsDominatorTree(BasicBlock* bb, 
                     const cfg::DominatorTree& DT,
                     std::vector<BasicBlock*>& order,
                     std::set<BasicBlock*>& visited) {
    if (!bb || visited.count(bb)) return;
    
    visited.insert(bb);
    order.push_back(bb);
    
    // 递归访问所有子节点
    const auto& children = DT.getChildren(bb);
    for (BasicBlock* child : children) {
        dfsDominatorTree(child, DT, order, visited);
    }
}

// 获取基本块在支配树中的深度优先遍历顺序
std::vector<BasicBlock*> GetDominatorTreeOrder(Function& F) {
    std::vector<BasicBlock*> order;
    std::set<BasicBlock*> visited;
    
    // 构建支配树
    cfg::DominatorTree DT;
    DT.runOnFunction(F);
    
    // 从入口块开始深度优先遍历
    BasicBlock* entry = F.getEntryBlock();
    if (entry) {
        dfsDominatorTree(entry, DT, order, visited);
    }
    
    // 确保所有基本块都被包含（处理不可达的块）
    for (auto& bb_ptr : F.basicBlocks) {
        if (!visited.count(bb_ptr.get())) {
            order.push_back(bb_ptr.get());
        }
    }
    
    return order;
}

// 基本块排序优化：基于支配树顺序重新排列基本块
void OrderBlocksByDominatorTree(Function& F) {
    if (F.basicBlocks.empty()) return;
    
    std::cout << "[BlockOrdering] 开始基本块排序优化..." << std::endl;
    
    // 获取基于支配树的顺序
    std::vector<BasicBlock*> newOrder = GetDominatorTreeOrder(F);
    
    // 按照新顺序重新排列基本块
    std::vector<std::unique_ptr<BasicBlock>> reorderedBlocks;
    reorderedBlocks.reserve(F.basicBlocks.size());
    
    // 按照支配树顺序添加基本块
    for (BasicBlock* bb : newOrder) {
        // 找到对应的unique_ptr并移动
        auto it = std::find_if(F.basicBlocks.begin(), F.basicBlocks.end(),
                              [bb](const std::unique_ptr<BasicBlock>& bb_ptr) {
                                  return bb_ptr.get() == bb;
                              });
        if (it != F.basicBlocks.end()) {
            reorderedBlocks.push_back(std::move(*it));
        }
    }
    
    // 替换原有的基本块列表
    F.basicBlocks = std::move(reorderedBlocks);
    
    // 重新构建CFG关系（因为基本块顺序改变了）
    cfg::buildCFG(F);
    
    std::cout << "[BlockOrdering] 基本块排序优化完成" << std::endl;
    
    // 调试输出：显示新的基本块顺序
    std::cout << "[BlockOrdering] 新的基本块顺序:" << std::endl;
    for (size_t i = 0; i < F.basicBlocks.size(); ++i) {
        std::cout << "  " << i << ": %" << F.basicBlocks[i]->label << std::endl;
    }
}

} // namespace llvm_ir 