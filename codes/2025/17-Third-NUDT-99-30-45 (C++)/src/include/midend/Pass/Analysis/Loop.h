#pragma once

#include "Dom.h"
#include "IR.h"
#include "Pass.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <vector>

namespace sysy {

// 前向声明
class LoopAnalysisResult;
class AliasAnalysisResult;
class SideEffectAnalysisResult;

/**
 * @brief 表示一个识别出的循环。
 */
class Loop {
private:
  static int NextLoopID; // 静态变量用于分配唯一ID
  int LoopID;
public:
  // 构造函数：指定循环头
  Loop(BasicBlock *header) : Header(header), LoopID(NextLoopID++) {}

  // 获取循环头
  BasicBlock *getHeader() const { return Header; }

  // 获取循环的名称 （基于ID）
  std::string getName() const { return "loop_" + std::to_string(LoopID); }
  // 获取循环体包含的所有基本块
  const std::set<BasicBlock *> &getBlocks() const { return LoopBlocks; }

  // 获取循环的出口基本块（即从循环内部跳转到循环外部的基本块）
  const std::set<BasicBlock *> &getExitBlocks() const { return ExitBlocks; }

  // 获取循环前置块（如果存在），可以为 nullptr
  BasicBlock *getPreHeader() const { return PreHeader; }

  // 获取直接包含此循环的父循环（如果存在），可以为 nullptr
  Loop *getParentLoop() const { return ParentLoop; }

  // 获取直接嵌套在此循环内的子循环
  const std::vector<Loop *> &getNestedLoops() const { return NestedLoops; }

  // 获取循环的层级 (0 表示最外层循环，1 表示嵌套一层，以此类推)
  int getLoopLevel() const { return Level; }

  // 检查一个基本块是否属于当前循环
  bool contains(BasicBlock *BB) const { return LoopBlocks.count(BB); }

  // 判断当前循环是否是最内层循环 (没有嵌套子循环)
  bool isInnermost() const { return NestedLoops.empty(); }

  // 获取循环的深度（从最外层开始计算）
  int getLoopDepth() const { return Level + 1; }

  // 获取循环体的大小（基本块数量）
  size_t getLoopSize() const { return LoopBlocks.size(); }

  // 检查循环是否有唯一的外部前驱（即是否有前置块）
  bool hasUniquePreHeader() const { return PreHeader != nullptr; }

  // 检查循环是否是最外层循环（没有父循环）
  bool isOutermost() const { return getParentLoop() == nullptr; }

  // 获取循环的所有出口（从循环内到循环外的基本块）
  std::vector<BasicBlock*> getExitingBlocks() const {
    std::vector<BasicBlock*> exitingBlocks;
    for (BasicBlock* bb : LoopBlocks) {
      for (BasicBlock* succ : bb->getSuccessors()) {
        if (!contains(succ)) {
          exitingBlocks.push_back(bb);
          break; // 每个基本块只添加一次
        }
      }
    }
    return exitingBlocks;
  }

  // 判断循环是否是简单循环（只有一个回边）
  bool isSimpleLoop() const {
    int backEdgeCount = 0;
    for (BasicBlock* pred : Header->getPredecessors()) {
      if (contains(pred)) {
        backEdgeCount++;
      }
    }
    return backEdgeCount == 1;
  }

  /**
   * 获取所有出口目标块 (循环外接收循环出口边的块)
   * 使用场景: 循环后置处理、phi节点分析
   */
  std::vector<BasicBlock*> getExitTargetBlocks() const {
    std::set<BasicBlock*> exitTargetSet;
    for (BasicBlock* bb : LoopBlocks) {
      for (BasicBlock* succ : bb->getSuccessors()) {
        if (!contains(succ)) {
          exitTargetSet.insert(succ);
        }
      }
    }
    return std::vector<BasicBlock*>(exitTargetSet.begin(), exitTargetSet.end());
  }

  /**
   * 计算循环的"深度"相对于指定的祖先循环
   * 使用场景: 相对深度计算、嵌套分析
   */
  int getRelativeDepth(Loop* ancestor) const {
    if (this == ancestor) return 0;
    
    int depth = 0;
    Loop* current = this->ParentLoop;
    while (current && current != ancestor) {
      depth++;
      current = current->ParentLoop;
    }
    
    return current == ancestor ? depth : -1; // -1表示不是祖先关系
  }

  /**
   * 检查循环是否包含函数调用
   * 使用场景: 内联决策、副作用分析
   */
  bool containsFunctionCalls() const {
    for (BasicBlock* bb : LoopBlocks) {
      for (auto& inst : bb->getInstructions()) {
        if (dynamic_cast<CallInst*>(inst.get())) {
          return true;
        }
      }
    }
    return false;
  }

  /**
   * 检查循环是否可能有副作用（基于副作用分析结果）
   * 使用场景: 循环优化决策、并行化分析
   */
  bool mayHaveSideEffects(SideEffectAnalysisResult* sideEffectAnalysis) const;

  /**
   * 检查循环是否访问全局内存（基于别名分析结果）
   * 使用场景: 并行化分析、缓存优化
   */
  bool accessesGlobalMemory(AliasAnalysisResult* aliasAnalysis) const;

  /**
   * 检查循环是否有可能的内存别名冲突
   * 使用场景: 向量化分析、并行化决策
   */
  bool hasMemoryAliasConflicts(AliasAnalysisResult* aliasAnalysis) const;

  /**
   * 估算循环的"热度" (基于嵌套深度和大小)
   * 使用场景: 优化优先级、资源分配
   */
  double getLoopHotness() const {
    // 简单的热度估算: 深度权重 + 大小惩罚
    double hotness = std::pow(2.0, Level); // 深度越深越热
    hotness /= std::sqrt(LoopBlocks.size()); // 大小越大相对热度降低
    return hotness;
  }

  // --- 供 LoopAnalysisPass 内部调用的方法，用于构建 Loop 对象 ---
  void addBlock(BasicBlock *BB) { LoopBlocks.insert(BB); }
  void addExitBlock(BasicBlock *BB) { ExitBlocks.insert(BB); }
  void setPreHeader(BasicBlock *BB) { PreHeader = BB; }
  void setParentLoop(Loop *loop) { ParentLoop = loop; }
  void addNestedLoop(Loop *loop) { NestedLoops.push_back(loop); }
  void setLoopLevel(int level) { Level = level; }
  void clearNestedLoops() { NestedLoops.clear(); }
private:
  BasicBlock *Header;                // 循环头基本块
  std::set<BasicBlock *> LoopBlocks; // 循环体包含的基本块集合
  std::set<BasicBlock *> ExitBlocks; // 循环出口基本块集合
  BasicBlock *PreHeader = nullptr;   // 循环前置块 (Optional)
  Loop *ParentLoop = nullptr;        // 父循环 (用于嵌套)
  std::vector<Loop *> NestedLoops;   // 嵌套的子循环
  int Level = -1;                    // 循环的层级，-1表示未计算
};

/**
 * @brief 循环分析结果类。
 * 包含一个函数中所有识别出的循环，并提供高效的查询缓存机制。
 */
class LoopAnalysisResult : public AnalysisResultBase {
public:
  LoopAnalysisResult(Function *F) : AssociatedFunction(F) {}
  ~LoopAnalysisResult() override = default;

  // ========== 缓存统计结构 ==========
  struct CacheStats {
    size_t innermostLoopsCached;
    size_t outermostLoopsCached;
    size_t loopsByDepthCached;
    size_t containingLoopsCached;
    size_t allNestedLoopsCached;
    size_t totalCachedQueries;
  };

private:
  // ========== 高频查询缓存 ==========
  mutable std::optional<std::vector<Loop*>> cachedInnermostLoops;
  mutable std::optional<std::vector<Loop*>> cachedOutermostLoops;
  mutable std::optional<int> cachedMaxDepth;
  mutable std::optional<size_t> cachedLoopCount;
  mutable std::map<int, std::vector<Loop*>> cachedLoopsByDepth;
  
  // ========== 中频查询缓存 ==========
  mutable std::map<BasicBlock*, Loop*> cachedInnermostContainingLoop;
  mutable std::map<Loop*, std::set<Loop*>> cachedAllNestedLoops; // 递归嵌套
  mutable std::map<BasicBlock*, std::vector<Loop*>> cachedAllContainingLoops;
  
  // ========== 缓存状态管理 ==========
  mutable bool cacheValid = true;

  // 内部辅助方法
  void invalidateCache() const {
    cachedInnermostLoops.reset();
    cachedOutermostLoops.reset();
    cachedMaxDepth.reset();
    cachedLoopCount.reset();
    cachedLoopsByDepth.clear();
    cachedInnermostContainingLoop.clear();
    cachedAllNestedLoops.clear();
    cachedAllContainingLoops.clear();
    cacheValid = false;
  }
  
  void ensureCacheValid() const {
    if (!cacheValid) {
      // 重新计算基础缓存
      computeBasicCache();
      cacheValid = true;
    }
  }
  
  void computeBasicCache() const {
    // 计算最内层循环
    if (!cachedInnermostLoops) {
      cachedInnermostLoops = std::vector<Loop*>();
      for (const auto& loop : AllLoops) {
        if (loop->isInnermost()) {
          cachedInnermostLoops->push_back(loop.get());
        }
      }
    }
    
    // 计算最外层循环
    if (!cachedOutermostLoops) {
      cachedOutermostLoops = std::vector<Loop*>();
      for (const auto& loop : AllLoops) {
        if (loop->isOutermost()) {
          cachedOutermostLoops->push_back(loop.get());
        }
      }
    }
    
    // 计算最大深度
    if (!cachedMaxDepth) {
      int maxDepth = 0;
      for (const auto& loop : AllLoops) {
        maxDepth = std::max(maxDepth, loop->getLoopDepth());
      }
      cachedMaxDepth = maxDepth;
    }
    
    // 计算循环总数
    if (!cachedLoopCount) {
      cachedLoopCount = AllLoops.size();
    }
  }

public:
  // ========== 基础接口 ==========

  // 添加一个识别出的循环到结果中
  void addLoop(std::unique_ptr<Loop> loop) {
    invalidateCache(); // 添加新循环时失效缓存
    AllLoops.push_back(std::move(loop));
    LoopMap[AllLoops.back()->getHeader()] = AllLoops.back().get();
  }

  // 获取所有识别出的循环（unique_ptr 管理内存）
  const std::vector<std::unique_ptr<Loop>> &getAllLoops() const { return AllLoops; }

  // ========== 高频查询接口 ==========
  
  /**
   * 获取所有最内层循环 - 循环优化的主要目标
   * 使用场景: 循环展开、向量化、循环不变量外提
   */
  const std::vector<Loop*>& getInnermostLoops() const {
    ensureCacheValid();
    if (!cachedInnermostLoops) {
      cachedInnermostLoops = std::vector<Loop*>();
      for (const auto& loop : AllLoops) {
        if (loop->isInnermost()) {
          cachedInnermostLoops->push_back(loop.get());
        }
      }
    }
    return *cachedInnermostLoops;
  }
  
  /**
   * 获取所有最外层循环
   * 使用场景: 循环树遍历、整体优化策略
   */
  const std::vector<Loop*>& getOutermostLoops() const {
    ensureCacheValid();
    if (!cachedOutermostLoops) {
      cachedOutermostLoops = std::vector<Loop*>();
      for (const auto& loop : AllLoops) {
        if (loop->isOutermost()) {
          cachedOutermostLoops->push_back(loop.get());
        }
      }
    }
    return *cachedOutermostLoops;
  }
  
  /**
   * 获取指定深度的所有循环
   * 使用场景: 分层优化、循环展开决策、并行化分析
   */
  const std::vector<Loop*>& getLoopsAtDepth(int depth) const {
    ensureCacheValid();
    if (cachedLoopsByDepth.find(depth) == cachedLoopsByDepth.end()) {
      std::vector<Loop*> result;
      for (const auto& loop : AllLoops) {
        if (loop->getLoopDepth() == depth) {
          result.push_back(loop.get());
        }
      }
      cachedLoopsByDepth[depth] = std::move(result);
    }
    return cachedLoopsByDepth[depth];
  }
  
  /**
   * 获取最大循环嵌套深度
   * 使用场景: 优化预算分配、编译时间控制
   */
  int getMaxLoopDepth() const {
    ensureCacheValid();
    if (!cachedMaxDepth) {
      int maxDepth = 0;
      for (const auto& loop : AllLoops) {
        maxDepth = std::max(maxDepth, loop->getLoopDepth());
      }
      cachedMaxDepth = maxDepth;
    }
    return *cachedMaxDepth;
  }
  
  /**
   * 获取循环总数
   * 使用场景: 统计信息、优化决策
   */
  size_t getLoopCount() const {
    ensureCacheValid();
    if (!cachedLoopCount) {
      cachedLoopCount = AllLoops.size();
    }
    return *cachedLoopCount;
  }

  // 获取指定深度的循环数量
  size_t getLoopCountAtDepth(int depth) const {
    return getLoopsAtDepth(depth).size();
  }

  // 检查函数是否包含循环
  bool hasLoops() const { return !AllLoops.empty(); }

  // ========== 中频查询接口 ==========
  
  /**
   * 获取包含指定基本块的最内层循环
   * 使用场景: 活跃性分析、寄存器分配、指令调度
   */
  Loop* getInnermostContainingLoop(BasicBlock* BB) const {
    ensureCacheValid();
    if (cachedInnermostContainingLoop.find(BB) == cachedInnermostContainingLoop.end()) {
      Loop* result = nullptr;
      int maxDepth = -1;
      for (const auto& loop : AllLoops) {
        if (loop->contains(BB) && loop->getLoopDepth() > maxDepth) {
          result = loop.get();
          maxDepth = loop->getLoopDepth();
        }
      }
      cachedInnermostContainingLoop[BB] = result;
    }
    return cachedInnermostContainingLoop[BB];
  }
  
  /**
   * 获取包含指定基本块的所有循环 (从外到内排序)
   * 使用场景: 循环间优化、依赖分析
   */
  const std::vector<Loop*>& getAllContainingLoops(BasicBlock* BB) const {
    ensureCacheValid();
    if (cachedAllContainingLoops.find(BB) == cachedAllContainingLoops.end()) {
      std::vector<Loop*> result;
      for (const auto& loop : AllLoops) {
        if (loop->contains(BB)) {
          result.push_back(loop.get());
        }
      }
      // 按深度排序 (外层到内层)
      std::sort(result.begin(), result.end(), 
                [](Loop* a, Loop* b) { return a->getLoopDepth() < b->getLoopDepth(); });
      cachedAllContainingLoops[BB] = std::move(result);
    }
    return cachedAllContainingLoops[BB];
  }
  
  /**
   * 获取指定循环的所有嵌套子循环 (递归)
   * 使用场景: 循环树分析、嵌套优化
   */
  const std::set<Loop*>& getAllNestedLoops(Loop* loop) const {
    ensureCacheValid();
    if (cachedAllNestedLoops.find(loop) == cachedAllNestedLoops.end()) {
      std::set<Loop*> result;
      std::function<void(Loop*)> collectNested = [&](Loop* current) {
        for (Loop* nested : current->getNestedLoops()) {
          result.insert(nested);
          collectNested(nested); // 递归收集
        }
      };
      collectNested(loop);
      cachedAllNestedLoops[loop] = std::move(result);
    }
    return cachedAllNestedLoops[loop];
  }

  // ========== 利用别名和副作用分析的查询接口 ==========
  
  /**
   * 获取所有纯循环（无副作用的循环）
   * 并行化、循环优化
   */
  std::vector<Loop*> getPureLoops(SideEffectAnalysisResult* sideEffectAnalysis) const {
    std::vector<Loop*> result;
    if (!sideEffectAnalysis) return result;
    
    for (const auto& loop : AllLoops) {
      if (!loop->mayHaveSideEffects(sideEffectAnalysis)) {
        result.push_back(loop.get());
      }
    }
    return result;
  }
  
  /**
   * 获取所有只访问局部内存的循环
   * 缓存优化、局部性分析
   */
  std::vector<Loop*> getLocalMemoryLoops(AliasAnalysisResult* aliasAnalysis) const {
    std::vector<Loop*> result;
    if (!aliasAnalysis) return result;
    
    for (const auto& loop : AllLoops) {
      if (!loop->accessesGlobalMemory(aliasAnalysis)) {
        result.push_back(loop.get());
      }
    }
    return result;
  }
  
  /**
   * 获取所有无内存别名冲突的循环
   * 向量化、并行化
   */
  std::vector<Loop*> getNoAliasConflictLoops(AliasAnalysisResult* aliasAnalysis) const {
    std::vector<Loop*> result;
    if (!aliasAnalysis) return result;
    
    for (const auto& loop : AllLoops) {
      if (!loop->hasMemoryAliasConflicts(aliasAnalysis)) {
        result.push_back(loop.get());
      }
    }
    return result;
  }

  // ========== 低频查询接口(不缓存) ==========
  
  /**
   * 检查两个循环是否有嵌套关系
   * 循环间依赖分析
   */
  bool isNestedLoop(Loop* inner, Loop* outer) const {
    if (inner == outer) return false;
    
    Loop* current = inner->getParentLoop();
    while (current) {
      if (current == outer) return true;
      current = current->getParentLoop();
    }
    return false;
  }
  
  /**
   * 获取两个循环的最近公共祖先循环
   * 循环融合分析、优化范围确定
   */
  Loop* getLowestCommonAncestor(Loop* loop1, Loop* loop2) const {
    if (!loop1 || !loop2) return nullptr;
    if (loop1 == loop2) return loop1;
    
    // 收集loop1的所有祖先
    std::set<Loop*> ancestors1;
    Loop* current = loop1;
    while (current) {
      ancestors1.insert(current);
      current = current->getParentLoop();
    }
    
    // 查找loop2祖先链中第一个在ancestors1中的循环
    current = loop2;
    while (current) {
      if (ancestors1.count(current)) {
        return current;
      }
      current = current->getParentLoop();
    }
    
    return nullptr; // 没有公共祖先
  }

  // 通过循环头获取 Loop 对象
  Loop *getLoopForHeader(BasicBlock *header) const {
    auto it = LoopMap.find(header);
    return (it != LoopMap.end()) ? it->second : nullptr;
  }

  // 通过某个基本块获取包含它的最内层循环 (向后兼容接口)
  Loop *getLoopContainingBlock(BasicBlock *BB) const {
    return getInnermostContainingLoop(BB);
  }

  // ========== 缓存管理接口 ==========
  
  /**
   * 手动失效缓存 (可删除)
   */
  void invalidateQueryCache() const {
    invalidateCache();
  }
  
  /**
   * 获取缓存统计信息
   */
  CacheStats getCacheStats() const {
    CacheStats stats = {};
    stats.innermostLoopsCached = cachedInnermostLoops.has_value() ? 1 : 0;
    stats.outermostLoopsCached = cachedOutermostLoops.has_value() ? 1 : 0;
    stats.loopsByDepthCached = cachedLoopsByDepth.size();
    stats.containingLoopsCached = cachedInnermostContainingLoop.size();
    stats.allNestedLoopsCached = cachedAllNestedLoops.size();
    stats.totalCachedQueries = stats.innermostLoopsCached + stats.outermostLoopsCached + 
                               stats.loopsByDepthCached + stats.containingLoopsCached + 
                               stats.allNestedLoopsCached;
    return stats;
  }

  // 打印分析结果
  void print() const;
  void printBBSet(const std::string &prefix, const std::set<BasicBlock *> &s) const;
  void printLoopVector(const std::string &prefix, const std::vector<Loop *> &loops) const;

private:
  Function *AssociatedFunction;                // 结果关联的函数
  std::vector<std::unique_ptr<Loop>> AllLoops; // 所有识别出的循环
  std::map<BasicBlock *, Loop *> LoopMap;      // 循环头到 Loop* 的映射，方便查找
};

/**
 * @brief 循环分析遍。
 * 识别函数中的所有循环，并生成 LoopAnalysisResult。
 */
class LoopAnalysisPass : public AnalysisPass {
public:
  // 唯一的 Pass ID，需要在 .cpp 文件中定义
  static void *ID;

  LoopAnalysisPass() : AnalysisPass("LoopAnalysis", Pass::Granularity::Function) {}

  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 核心运行方法：在每个函数上执行循环分析
  bool runOnFunction(Function *F, AnalysisManager &AM) override;

  // 获取分析结果
  std::unique_ptr<AnalysisResultBase> getResult() override { return std::move(CurrentResult); }

private:
  std::unique_ptr<LoopAnalysisResult> CurrentResult; // 当前函数的分析结果
};

} // namespace sysy