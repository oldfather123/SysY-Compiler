#pragma once

#include "Loop.h"          // 循环分析依赖
#include "Dom.h"           // 支配树分析依赖
#include "IR.h"            // IR定义
#include "IRBuilder.h"     // IR构建器
#include "Pass.h"          // Pass框架
#include <memory>
#include <set>
#include <vector>

namespace sysy {

/**
 * @brief 循环规范化转换Pass
 * 
 * 该Pass在循环不变量提升等优化前运行，主要负责：
 * 1. 为没有前置块(preheader)的循环创建前置块
 * 2. 确保循环结构符合后续优化的要求
 * 3. 规范化循环的控制流结构
 * 
 * 前置块的作用：
 * - 为循环不变量提升提供插入位置
 * - 简化循环分析和优化
 * - 确保循环有唯一的入口点
 */
class LoopNormalizationPass : public OptimizationPass {
public:
  // 唯一的 Pass ID
  static void *ID;

  LoopNormalizationPass(IRBuilder* builder) : OptimizationPass("LoopNormalization", Pass::Granularity::Function), builder(builder) {}

  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 核心运行方法
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  
  // 声明分析依赖和失效信息
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;

private:
  // ========== IR构建器 ==========
  IRBuilder* builder;                                  // IR构建器
  
  // ========== 缓存的分析结果 ==========
  LoopAnalysisResult* loopAnalysis;                    // 循环结构分析结果
  DominatorTree* domTree;                              // 支配树分析结果
  
  // ========== 规范化统计 ==========
  struct NormalizationStats {
    size_t totalLoops;                    // 总循环数
    size_t loopsNeedingPreheader;        // 需要前置块的循环数
    size_t preheadersCreated;            // 创建的前置块数
    size_t loopsNormalized;              // 规范化的循环数
    size_t redundantPhisRemoved;         // 删除的冗余PHI节点数
    
    NormalizationStats() : totalLoops(0), loopsNeedingPreheader(0), 
                          preheadersCreated(0), loopsNormalized(0), 
                          redundantPhisRemoved(0) {}
  } stats;
  
  // ========== 核心规范化方法 ==========
  
  /**
   * 规范化单个循环
   * @param loop 要规范化的循环
   * @return 是否进行了修改
   */
  bool normalizeLoop(Loop* loop);
  
  /**
   * 为循环创建前置块
   * @param loop 需要前置块的循环
   * @return 创建的前置块，如果失败则返回nullptr
   */
  BasicBlock* createPreheaderForLoop(Loop* loop);
  
  /**
   * 检查循环是否需要前置块（基于结构性需求）
   * @param loop 要检查的循环
   * @return true如果需要前置块
   */
  bool needsPreheader(Loop* loop);
  
  /**
   * 检查循环是否已有合适的前置块
   * @param loop 要检查的循环
   * @return 现有的前置块，如果没有则返回nullptr
   */
  BasicBlock* getExistingPreheader(Loop* loop);
  
  /**
   * 更新支配树关系（在创建新块后）
   * @param newBlock 新创建的基本块
   * @param loop 相关的循环
   */
  void updateDominatorRelations(BasicBlock* newBlock, Loop* loop);
  
  /**
   * 重定向循环外的前驱块到新的前置块
   * @param loop 目标循环
   * @param preheader 新创建的前置块
   * @param header 循环头部
   */
  void redirectExternalPredecessors(Loop* loop, BasicBlock* preheader, BasicBlock* header, const std::vector<BasicBlock*>& externalPreds);
  
  /**
   * 为前置块生成合适的名称
   * @param loop 相关的循环
   * @return 生成的前置块名称
   */
  std::string generatePreheaderName(Loop* loop);
  
  /**
   * 验证规范化结果的正确性
   * @param loop 规范化后的循环
   * @return true如果规范化正确
   */
  bool validateNormalization(Loop* loop);
  
  // ========== 辅助方法 ==========
  
  /**
   * 获取循环的外部前驱块（不在循环内的前驱）
   * @param loop 目标循环
   * @return 外部前驱块列表
   */
  std::vector<BasicBlock*> getExternalPredecessors(Loop* loop);
  
  /**
   * 检查基本块是否适合作为前置块
   * @param block 候选基本块
   * @param loop 目标循环
   * @return true如果适合作为前置块
   */
  bool isSuitableAsPreheader(BasicBlock* block, Loop* loop);
  
  /**
   * 更新PHI节点以适应新的前置块
   * @param header 循环头部
   * @param preheader 新的前置块
   * @param oldPreds 原来的外部前驱
   */
  void updatePhiNodesForPreheader(BasicBlock* header, BasicBlock* preheader,
                                 const std::vector<BasicBlock*>& oldPreds);
  
  /**
   * 打印规范化统计信息
   */
  void printStats(Function* F);
};

} // namespace sysy
