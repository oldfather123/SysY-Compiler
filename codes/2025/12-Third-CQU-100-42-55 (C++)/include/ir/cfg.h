#ifndef LLVM_IR_CFG_H
#define LLVM_IR_CFG_H

#include "llvm_ir.h"
#include <unordered_map>

namespace llvm_ir {
namespace cfg {

// 全局辅助函数声明
void rebuildUseDefChainsOnFunction(Function& F, bool debug); // 在function级别重建use-def链
void rebuildUseDefChainsModule(Module& M); // 在module级别重建use-def链
void buildCFG(Function& F); // 构建CFG关系

class DominatorTree {
public:
    // 计算结果的存储
    std::unordered_map<BasicBlock*, BasicBlock*> idomMap;  // (子 -> 父) 直接支配者
    // 在所有支配节点 N 的节点中，离 N 最近的那个节点 D，称为 N 的直接支配者
    std::unordered_map<BasicBlock*, std::set<BasicBlock*>> dominanceFrontiers;  // (块 -> 支配边界) 
    // N 的支配边界 DF(N) 是一个节点集合。集合中的每个节点 Y 都满足：N 支配 Y 的一个前驱，但 N 并不严格支配 Y（即 N 不支配 Y 或者 N=Y）
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> childrenMap;  // (块 -> 子节点) (父->子列表)

    bool dominates(BasicBlock* dom, BasicBlock* b) const; // 判断dom是否支配b

    void runOnFunction(Function& F, bool reverse = false); // 构建支配树, reverse=true时构建后向支配树

    // 获取一个块的直接支配节点
    BasicBlock* getIdom(BasicBlock* B) const {
        auto it = idomMap.find(B);
        return it != idomMap.end() ? it->second : nullptr;
    }

    // 获取一个块的支配边界
    const std::set<BasicBlock*>& getDominanceFrontier(BasicBlock* B) const {
        auto it = dominanceFrontiers.find(B);
        return it != dominanceFrontiers.end() ? it->second : *emptySet.get();
    }

    // 获取支配树中一个块的子节点
    const std::vector<BasicBlock*>& getChildren(BasicBlock* B) const {
        auto it = childrenMap.find(B);
        return it != childrenMap.end() ? it->second : *emptyChildren.get();
    }

private:
    void computeIdom(Function& F, bool reverse = false); // 计算直接支配者 (idomMap)
    BasicBlock* findCommonDominator(BasicBlock* a, BasicBlock* b);  // 找 a 和 b 在当前支配树上的 LCA (最近公共祖先)
    void computeDominanceFrontiers(Function& F); // 基于 idomMap 和支配树计算支配边界 (dominanceFrontiers)
    void buildTree(Function& F); // 将 idomMap（子->父）的关系反转，得到 childrenMap（父->子列表）(childrenMap)
    
    void swapCFG(Function& F); // 交换所有基本块的前驱和后继关系 (方便构建后向支配树)
    BasicBlock* getEntryBlock(Function& F, bool reverse = false); // 获取入口块（正向支配树）或出口块（后向支配树）

    std::unique_ptr<std::set<BasicBlock*>> emptySet = std::make_unique<std::set<BasicBlock*>>();
    std::unique_ptr<std::vector<BasicBlock*>> emptyChildren = std::make_unique<std::vector<BasicBlock*>>();
};

// 在CFG中插入一个中间块，用于处理PHI指令
/*B1    \                                   B1  \
    B2  -> B4   will be transformed to      B2  -> midB -> B5
    B3  /                                   B3  /

    this function will split the phi instructions and return midB

    %3 = phi [%0,L1],[%1,L2],[%2,L3]

    midB = L4 : %4 = phi [%0,L1],[%1,L2]
    %3 = phi [%4,L4],[%2,L3]
*/
BasicBlock* InsertTransferBlock(std::set<BasicBlock*>& froms, BasicBlock* to, bool insertAfterFrom = true);

} // namespace cfg
} // namespace llvm_ir

#endif // LLVM_IR_CFG_H