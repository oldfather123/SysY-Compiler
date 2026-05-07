#pragma once

#include "Pass.h"
#include "IR.h"
#include "LoopCharacteristics.h"
#include "Loop.h"
#include "Dom.h"
#include "SideEffectAnalysis.h"
#include "AliasAnalysis.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace sysy {

// 前向声明
class LoopCharacteristicsResult;
class LoopAnalysisResult;

/**
 * @brief 死归纳变量信息
 * 记录一个可以被消除的归纳变量
 */
struct DeadInductionVariable {
  PhiInst* phiInst;                    // phi 指令
  std::vector<Instruction*> relatedInsts; // 相关的递增/递减指令
  Loop* containingLoop;                // 所在循环
  bool canEliminate;                   // 是否可以安全消除
  
  DeadInductionVariable(PhiInst* phi, Loop* loop) 
    : phiInst(phi), containingLoop(loop), canEliminate(false) {}
};

/**
 * @brief 归纳变量消除上下文类
 * 封装归纳变量消除优化的核心逻辑和状态
 */
class InductionVariableEliminationContext {
public:
  InductionVariableEliminationContext() {}
  
  /**
   * 运行归纳变量消除优化
   * @param F 目标函数
   * @param AM 分析管理器
   * @return 是否修改了IR
   */
  bool run(Function* F, AnalysisManager& AM);

private:
  // 分析结果缓存
  LoopAnalysisResult* loopAnalysis = nullptr;
  LoopCharacteristicsResult* loopCharacteristics = nullptr;
  DominatorTree* dominatorTree = nullptr;
  SideEffectAnalysisResult* sideEffectAnalysis = nullptr;
  AliasAnalysisResult* aliasAnalysis = nullptr;
  
  // 死归纳变量存储
  std::vector<std::unique_ptr<DeadInductionVariable>> deadIVs;
  std::unordered_map<Loop*, std::vector<DeadInductionVariable*>> loopToDeadIVs;
  
  // ========== 核心分析和优化阶段 ==========
  
  /**
   * 阶段1：识别死归纳变量
   * 找出没有被有效使用的归纳变量
   */
  void identifyDeadInductionVariables(Function* F);
  
  /**
   * 阶段2：分析消除的安全性
   * 确保消除操作不会破坏程序语义
   */
  void analyzeSafetyForElimination();
  
  /**
   * 阶段3：执行归纳变量消除
   * 删除死归纳变量及其相关指令
   */
  bool performInductionVariableElimination();
  
  // ========== 辅助方法 ==========
  
  /**
   * 检查归纳变量是否为死归纳变量
   * @param iv 归纳变量信息
   * @param loop 所在循环
   * @return 如果是死归纳变量返回相关信息，否则返回nullptr
   */
  std::unique_ptr<DeadInductionVariable> 
  isDeadInductionVariable(const InductionVarInfo* iv, Loop* loop);
  
  /**
   * 递归分析phi指令及其使用链是否都是死代码
   * @param phiInst phi指令
   * @param loop 所在循环
   * @return phi指令是否可以安全删除
   */
  bool isPhiInstructionDeadRecursively(PhiInst* phiInst, Loop* loop);
  
  /**
   * 递归分析指令的使用链是否都是死代码
   * @param inst 要分析的指令
   * @param loop 所在循环
   * @param visited 已访问的指令集合（避免无限递归）
   * @param currentPath 当前递归路径（检测循环依赖）
   * @return 指令的使用链是否都是死代码
   */
  bool isInstructionUseChainDeadRecursively(Instruction* inst, Loop* loop, 
                                           std::set<Instruction*>& visited, 
                                           std::set<Instruction*>& currentPath);
  
  /**
   * 检查循环是否有副作用
   * @param loop 要检查的循环
   * @return 循环是否有副作用
   */
  bool loopHasSideEffects(Loop* loop);
  
  /**
   * 检查指令是否被用于循环退出条件
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @return 是否被用于循环退出条件
   */
  bool isUsedInLoopExitCondition(Instruction* inst, Loop* loop);
  
  /**
   * 检查指令的结果是否未被有效使用
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @return 指令结果是否未被有效使用
   */
  bool isInstructionResultUnused(Instruction* inst, Loop* loop);
  
  /**
   * 检查store指令是否存储到死地址（利用别名分析）
   * @param store store指令
   * @param loop 所在循环  
   * @return 是否存储到死地址
   */
  bool isStoreToDeadLocation(StoreInst* store, Loop* loop);
  
  /**
   * 检查指令是否为死代码或只在循环内部使用
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @return 是否为死代码或只在循环内部使用
   */
  bool isInstructionDeadOrInternalOnly(Instruction* inst, Loop* loop);
  
  /**
   * 检查指令是否有效地为死代码（带递归深度限制）
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @param maxDepth 最大递归深度
   * @return 指令是否有效地为死代码
   */
  bool isInstructionEffectivelyDead(Instruction* inst, Loop* loop, int maxDepth);
  
  /**
   * 检查store指令是否有后续的load操作
   * @param store store指令
   * @param loop 所在循环
   * @return 是否有后续的load操作
   */
  bool hasSubsequentLoad(StoreInst* store, Loop* loop);
  
  /**
   * 检查指令是否在循环外有使用
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @return 是否在循环外有使用
   */
  bool hasUsageOutsideLoop(Instruction* inst, Loop* loop);
  
  /**
   * 检查store指令是否在循环外有后续的load操作
   * @param store store指令
   * @param loop 所在循环
   * @return 是否在循环外有后续的load操作
   */
  bool hasSubsequentLoadOutsideLoop(StoreInst* store, Loop* loop);
  
  /**
   * 递归检查基本块子树中是否有对指定位置的load操作
   * @param bb 基本块
   * @param ptr 指针
   * @param visited 已访问的基本块集合
   * @return 是否有load操作
   */
  bool hasLoadInSubtree(BasicBlock* bb, Value* ptr, std::set<BasicBlock*>& visited);
  
  /**
   * 收集与归纳变量相关的所有指令
   * @param phiInst phi指令
   * @param loop 所在循环
   * @return 相关指令列表
   */
  std::vector<Instruction*> collectRelatedInstructions(PhiInst* phiInst, Loop* loop);
  
  /**
   * 检查消除归纳变量的安全性
   * @param deadIV 死归纳变量
   * @return 是否可以安全消除
   */
  bool isSafeToEliminate(const DeadInductionVariable* deadIV);
  
  /**
   * 消除单个死归纳变量
   * @param deadIV 死归纳变量
   * @return 是否成功消除
   */
  bool eliminateDeadInductionVariable(DeadInductionVariable* deadIV);
  
  /**
   * 打印调试信息
   */
  void printDebugInfo();
};

/**
 * @brief 归纳变量消除优化遍
 * 消除循环中无用的归纳变量，减少寄存器压力
 */
class InductionVariableElimination : public OptimizationPass {
public:
  // 唯一的 Pass ID
  static void *ID;
  
  InductionVariableElimination() 
    : OptimizationPass("InductionVariableElimination", Granularity::Function) {}
  
  /**
   * 在函数上运行归纳变量消除优化
   * @param F 目标函数
   * @param AM 分析管理器
   * @return 是否修改了IR
   */
  bool runOnFunction(Function* F, AnalysisManager& AM) override;
  
  /**
   * 声明分析依赖和失效信息
   */
  void getAnalysisUsage(std::set<void*>& analysisDependencies, 
                       std::set<void*>& analysisInvalidations) const override;
  
  void* getPassID() const override { return &ID; }
};

} // namespace sysy
