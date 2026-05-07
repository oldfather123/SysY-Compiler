#pragma once

#include "Pass.h" // 包含 Pass 框架
#include "IR.h"   // 包含 IR 定义
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <functional>

namespace sysy {

// 支配树分析结果类
class DominatorTree : public AnalysisResultBase {
public:
    DominatorTree(Function* F);
    // 获取指定基本块的所有支配者
    const std::set<BasicBlock*>* getDominators(BasicBlock* BB) const;
    // 获取指定基本块的即时支配者 (Immediate Dominator)
    BasicBlock* getImmediateDominator(BasicBlock* BB) const;  
    // 获取指定基本块的支配边界 (Dominance Frontier)
    const std::set<BasicBlock*>* getDominanceFrontier(BasicBlock* BB) const;   
    // 获取指定基本块在支配树中的子节点
    const std::set<BasicBlock*>* getDominatorTreeChildren(BasicBlock* BB) const;
    // 额外的 Getter：获取所有支配者、即时支配者和支配边界的完整映射（可选，主要用于调试或特定场景）
    const std::map<BasicBlock*, std::set<BasicBlock*>>& getDominatorsMap() const { return Dominators; }
    const std::map<BasicBlock*, BasicBlock*>& getIDomsMap() const { return IDoms; }
    const std::map<BasicBlock*, std::set<BasicBlock*>>& getDominanceFrontiersMap() const { return DominanceFrontiers; }

    // 计算所有基本块的支配者集合
    void computeDominators(Function* F);
    // 计算所有基本块的即时支配者（内部使用 Lengauer-Tarjan 算法）
    void computeIDoms(Function* F); 
    // 计算所有基本块的支配边界
    void computeDominanceFrontiers(Function* F);
    // 计算支配树的结构（即每个节点的直接子节点）
    void computeDominatorTreeChildren(Function* F);
private:
    // 与该支配树关联的函数
    Function* AssociatedFunction;
    std::map<BasicBlock*, std::set<BasicBlock*>> Dominators;       // 每个基本块的支配者集合
    std::map<BasicBlock*, BasicBlock*> IDoms;                      // 每个基本块的即时支配者
    std::map<BasicBlock*, std::set<BasicBlock*>> DominanceFrontiers; // 每个基本块的支配边界
    std::map<BasicBlock*, std::set<BasicBlock*>> DominatorTreeChildren; // 支配树中每个基本块的子节点

    // ==========================================================
    // Lengauer-Tarjan 算法内部所需的数据结构和辅助函数
    // 这些成员是私有的，以封装 LT 算法的复杂性并避免命名空间污染
    // ==========================================================

    // DFS 遍历相关：
    std::map<BasicBlock*, int> dfnum_map;            // 存储每个基本块的 DFS 编号
    std::vector<BasicBlock*> vertex_vec;             // 通过 DFS 编号反向查找对应的基本块指针
    std::map<BasicBlock*, BasicBlock*> parent_map;   // 存储 DFS 树中每个基本块的父节点
    int df_counter;                                  // DFS 计数器，也代表 DFS 遍历的总节点数 (N)

    // 半支配者 (Semi-dominator) 相关：
    std::map<BasicBlock*, BasicBlock*> sdom_map;     // 存储每个基本块的半支配者
    std::map<BasicBlock*, BasicBlock*> idom_map;     // 存储每个基本块的即时支配者 (IDom)
    std::map<BasicBlock*, std::vector<BasicBlock*>> bucket_map; // 桶结构，用于存储具有相同半支配者的节点，以延迟 IDom 计算

    // 并查集 (Union-Find) 相关（用于 evalAndCompress 函数）：
    std::map<BasicBlock*, BasicBlock*> ancestor_map; // 并查集中的父节点（用于路径压缩）
    std::map<BasicBlock*, BasicBlock*> label_map;    // 并查集中，每个集合的代表节点（或其路径上 sdom 最小的节点）

    // ==========================================================
    // 辅助计算函数 (私有)
    // ==========================================================

    // 计算基本块的逆后序遍历 (Reverse Post Order, RPO) 顺序
    // RPO 用于优化支配者计算和 LT 算法的效率
    std::vector<BasicBlock*> computeReversePostOrder(Function* F);

    // Lengauer-Tarjan 算法特定的辅助 DFS 函数
    // 用于初始化 dfnum_map, vertex_vec, parent_map
    void dfs_lt_helper(BasicBlock* u);                 
    
    // 结合了并查集的 Find 操作和 LT 算法的 Eval 操作
    // 用于在路径压缩时更新 label，找到路径上 sdom 最小的节点
    BasicBlock* evalAndCompress_lt_helper(BasicBlock* i); 
    
    // 并查集的 Link 操作
    // 将 v_child 挂载到 u_parent 的并查集树下
    void link_lt_helper(BasicBlock* u_parent, BasicBlock* v_child); 
};


// 支配树分析遍
class DominatorTreeAnalysisPass : public AnalysisPass {
public:
    // 唯一的 Pass ID
    static void *ID;

    DominatorTreeAnalysisPass() : AnalysisPass("DominatorTreeAnalysis", Pass::Granularity::Function) {}

    // 实现 getPassID
    void* getPassID() const override { return &ID; }

    bool runOnFunction(Function* F, AnalysisManager &AM) override;

    std::unique_ptr<AnalysisResultBase> getResult() override;

private:
    std::unique_ptr<DominatorTree> CurrentDominatorTree;
};

} // namespace sysy