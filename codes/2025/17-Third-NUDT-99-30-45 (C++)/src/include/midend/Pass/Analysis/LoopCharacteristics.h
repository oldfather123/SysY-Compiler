#pragma once

#include "Dom.h"         // 支配树分析依赖
#include "Loop.h"        // 循环分析依赖
#include "Liveness.h"    // 活跃性分析依赖
#include "AliasAnalysis.h" // 别名分析依赖
#include "SideEffectAnalysis.h" // 副作用分析依赖
#include "CallGraphAnalysis.h" // 调用图分析依赖
#include "IR.h"          // IR定义
#include "Pass.h"        // Pass框架
#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace sysy {

// 前向声明
class LoopCharacteristicsResult;

enum IVKind {
  kBasic,        // 基本归纳变量
  kLinear,     // 线性归纳变量
  kCmplx       // 复杂派生归纳变量
} ;          // 归纳变量类型

struct InductionVarInfo {
  Value* div;                  // 派生归纳变量的指令
  Value* base = nullptr;                 // 其根phi或BIV或DIV
  std::pair<Value*, Value*> Multibase = {nullptr, nullptr}; // 多个BIV
  Instruction::Kind Instkind;      // 操作类型
  int factor = 1;              // 系数（如i*2+3的2）
  int offset = 0;              // 常量偏移
  bool valid;    // 是否线性可归约
  IVKind ivkind;          // 归纳变量类型


static std::unique_ptr<InductionVarInfo> createBasicBIV(Value* v, Instruction::Kind kind, Value* base = nullptr, int factor = 1, int offset = 0) {
  return std::make_unique<InductionVarInfo>(
    InductionVarInfo{v, base, {nullptr, nullptr}, kind, factor, offset, true, IVKind::kBasic}
  );
}

static std::unique_ptr<InductionVarInfo> createSingleDIV(Value* v, Instruction::Kind kind, Value* base = nullptr, int factor = 1, int offset = 0) {
  return std::make_unique<InductionVarInfo>(
    InductionVarInfo{v, base, {nullptr, nullptr}, kind, factor, offset, true,  IVKind::kLinear}
  );
}

static std::unique_ptr<InductionVarInfo> createDoubleDIV(Value* v, Instruction::Kind kind, Value* base1 = nullptr,  Value* base2 = nullptr, int factor = 1, int offset = 0) {
  return std::make_unique<InductionVarInfo>(
    InductionVarInfo{v, nullptr, {base1, base2}, kind, factor, offset, false, IVKind::kCmplx}
  );
}
};

/**
 * @brief 循环特征信息结构 - 基础循环分析阶段
 * 存储循环的基本特征信息，为后续精确分析提供基础
 */
struct LoopCharacteristics {
  Loop* loop;                                    // 关联的循环对象

  // ========== 基础循环形式分析 ==========
  bool isCountingLoop;                         // 是否为计数循环 (for i=0; i<n; i++)
  bool isSimpleForLoop;                        // 是否为简单for循环
  bool hasComplexControlFlow;                  // 是否有复杂控制流 (break, continue)
  bool isInnermost;                            // 是否为最内层循环
  
  // ========== 归纳变量分析 ==========
  
  // ========== 基础循环不变量分析 ==========
  std::unordered_set<Value*> loopInvariants;              // 循环不变量
  std::unordered_set<Instruction*> invariantInsts;       // 可提升的不变指令

  std::vector<std::unique_ptr<InductionVarInfo>> InductionVars;     // 归纳变量
  
  // ========== 基础边界分析 ==========
  std::optional<int> staticTripCount;          // 静态循环次数（如果可确定）
  bool hasKnownBounds;                         // 是否有已知边界
  
  // ========== 基础纯度和副作用分析 ==========
  bool isPure;                                 // 是否为纯循环（无副作用）
  bool accessesOnlyLocalMemory;                // 是否只访问局部内存
  bool hasNoMemoryAliasConflicts;              // 是否无内存别名冲突
  
  // ========== 基础内存访问模式分析 ==========
  struct MemoryAccessPattern {
    std::vector<Instruction*> loadInsts;       // load指令列表
    std::vector<Instruction*> storeInsts;      // store指令列表
    bool isArrayParameter;                     // 是否为数组参数访问
    bool isGlobalArray;                        // 是否为全局数组访问
    bool hasConstantIndices;                   // 是否使用常量索引
  };
  std::map<Value*, MemoryAccessPattern> memoryPatterns; // 内存访问模式
  
  // ========== 基础性能特征 ==========
  size_t instructionCount;                     // 循环体指令数
  size_t memoryOperationCount;                 // 内存操作数
  size_t arithmeticOperationCount;             // 算术操作数
  double computeToMemoryRatio;                 // 计算与内存操作比率
  
  // ========== 基础优化提示 ==========
  bool benefitsFromUnrolling;                  // 是否适合循环展开
  int suggestedUnrollFactor;                   // 建议的展开因子
  
  // 构造函数 - 简化的基础分析初始化
  LoopCharacteristics(Loop* l) : loop(l), 
    isCountingLoop(false), isSimpleForLoop(false), hasComplexControlFlow(false),
    isInnermost(false), hasKnownBounds(false), isPure(false), 
    accessesOnlyLocalMemory(false), hasNoMemoryAliasConflicts(false),
    benefitsFromUnrolling(false), suggestedUnrollFactor(1), 
    instructionCount(0), memoryOperationCount(0),
    arithmeticOperationCount(0), computeToMemoryRatio(0.0) {}
};

/**
 * @brief 循环特征分析结果类
 * 包含函数中所有循环的特征信息，并提供查询接口
 */
class LoopCharacteristicsResult : public AnalysisResultBase {
public:
  LoopCharacteristicsResult(Function *F) : AssociatedFunction(F) {}
  ~LoopCharacteristicsResult() override = default;

  // ========== 基础接口 ==========
  
  /**
   * 添加循环特征信息
   */
  void addLoopCharacteristics(std::unique_ptr<LoopCharacteristics> characteristics) {
    auto* loop = characteristics->loop;
    CharacteristicsMap[loop] = std::move(characteristics);
  }
  
  /**
   * 获取指定循环的特征信息
   */
  const LoopCharacteristics* getCharacteristics(Loop* loop) const {
    auto it = CharacteristicsMap.find(loop);
    return (it != CharacteristicsMap.end()) ? it->second.get() : nullptr;
  }
  
  /**
   * 获取所有循环特征信息
   */
  const std::map<Loop*, std::unique_ptr<LoopCharacteristics>>& getAllCharacteristics() const {
    return CharacteristicsMap;
  }

  // ========== 核心查询接口 ==========
  
  /**
   * 获取所有计数循环
   */
  std::vector<Loop*> getCountingLoops() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->isCountingLoop) {
        result.push_back(loop);
      }
    }
    return result;
  }
  
  /**
   * 获取所有纯循环（无副作用）
   */
  std::vector<Loop*> getPureLoops() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->isPure) {
        result.push_back(loop);
      }
    }
    return result;
  }
  
  /**
   * 获取所有只访问局部内存的循环
   */
  std::vector<Loop*> getLocalMemoryOnlyLoops() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->accessesOnlyLocalMemory) {
        result.push_back(loop);
      }
    }
    return result;
  }
  
  /**
   * 获取所有无内存别名冲突的循环
   */
  std::vector<Loop*> getNoAliasConflictLoops() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->hasNoMemoryAliasConflicts) {
        result.push_back(loop);
      }
    }
    return result;
  }
  
  /**
   * 获取所有适合展开的循环
   */
  std::vector<Loop*> getUnrollingCandidates() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->benefitsFromUnrolling) {
        result.push_back(loop);
      }
    }
    return result;
  }
  
  /**
   * 根据热度排序循环 (用于优化优先级)
   */
  std::vector<Loop*> getLoopsByHotness() const {
    std::vector<Loop*> result;
    for (const auto& [loop, chars] : CharacteristicsMap) {
      result.push_back(loop);
    }
    
    // 按循环热度排序 (嵌套深度 + 循环次数 + 指令数)
    std::sort(result.begin(), result.end(), [](Loop* a, Loop* b) {
      double hotnessA = a->getLoopHotness();
      double hotnessB = b->getLoopHotness();
      return hotnessA > hotnessB; // 降序排列
    });
    
    return result;
  }

  // ========== 基础统计接口 ==========
  
  /**
   * 获取基础优化统计信息
   */
  struct BasicOptimizationStats {
    size_t totalLoops;
    size_t countingLoops;
    size_t unrollingCandidates;
    size_t pureLoops;
    size_t localMemoryOnlyLoops;
    size_t noAliasConflictLoops;
    double avgInstructionCount;
    double avgComputeMemoryRatio;
  };
  
  BasicOptimizationStats getOptimizationStats() const {
    BasicOptimizationStats stats = {};
    stats.totalLoops = CharacteristicsMap.size();
    
    size_t totalInstructions = 0;
    double totalComputeMemoryRatio = 0.0;
    
    for (const auto& [loop, chars] : CharacteristicsMap) {
      if (chars->isCountingLoop) stats.countingLoops++;
      if (chars->benefitsFromUnrolling) stats.unrollingCandidates++;
      if (chars->isPure) stats.pureLoops++;
      if (chars->accessesOnlyLocalMemory) stats.localMemoryOnlyLoops++;
      if (chars->hasNoMemoryAliasConflicts) stats.noAliasConflictLoops++;
      
      totalInstructions += chars->instructionCount;
      totalComputeMemoryRatio += chars->computeToMemoryRatio;
    }
    
    if (stats.totalLoops > 0) {
      stats.avgInstructionCount = static_cast<double>(totalInstructions) / stats.totalLoops;
      stats.avgComputeMemoryRatio = totalComputeMemoryRatio / stats.totalLoops;
    }
    
    return stats;
  }

  // 打印分析结果
  void print() const;

private:
  Function *AssociatedFunction;                                    // 关联的函数
  std::map<Loop*, std::unique_ptr<LoopCharacteristics>> CharacteristicsMap; // 循环特征映射
};

/**
 * @brief 基础循环特征分析遍
 * 在循环规范化前执行，进行基础的循环特征分析，为后续精确分析提供基础
 */
class LoopCharacteristicsPass : public AnalysisPass {
public:
  // 唯一的 Pass ID
  static void *ID;

  LoopCharacteristicsPass() : AnalysisPass("LoopCharacteristics", Pass::Granularity::Function) {}

  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 核心运行方法
  bool runOnFunction(Function *F, AnalysisManager &AM) override;

  // 获取分析结果
  std::unique_ptr<AnalysisResultBase> getResult() override { return std::move(CurrentResult); }

private:
  std::unique_ptr<LoopCharacteristicsResult> CurrentResult;
  
  // ========== 缓存的分析结果 ==========
  LoopAnalysisResult* loopAnalysis;           // 循环结构分析结果
  AliasAnalysisResult* aliasAnalysis;         // 别名分析结果  
  SideEffectAnalysisResult* sideEffectAnalysis; // 副作用分析结果
  
  // ========== 核心分析方法 ==========
  void analyzeLoop(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础循环形式分析
  void analyzeLoopForm(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础性能指标计算
  void computePerformanceMetrics(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础纯度和副作用分析
  void analyzePurityAndSideEffects(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础归纳变量识别
  void identifyBasicInductionVariables(Loop* loop, LoopCharacteristics* characteristics);
  
  // 循环不变量识别
  void identifyBasicLoopInvariants(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础边界分析
  void analyzeBasicLoopBounds(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础内存访问模式分析
  void analyzeBasicMemoryAccessPatterns(Loop* loop, LoopCharacteristics* characteristics);
  
  // 基础优化评估
  void evaluateBasicOptimizationOpportunities(Loop* loop, LoopCharacteristics* characteristics);
  
  // ========== 辅助方法 ==========
  bool isClassicLoopInvariant(Value* val, Loop* loop, const std::unordered_set<Value*>& invariants);
  void findDerivedInductionVars(Value* root,
    Value* base, // 只传单一BIV base
    Loop* loop,
    std::vector<std::unique_ptr<InductionVarInfo>>& ivs,
    std::set<Value*>& visited
  );
  bool isBasicInductionVariable(Value* val, Loop* loop);
  // ========== 循环不变量分析辅助方法 ==========
  bool isInvariantOperands(Instruction* inst, Loop* loop, const std::unordered_set<Value*>& invariants);
  bool isMemoryLocationModifiedInLoop(Value* ptr, Loop* loop);
  bool isMemoryLocationLoadedInLoop(Value* ptr, Loop* loop, Instruction* excludeInst = nullptr);
  bool isPureFunction(Function* calledFunc);
};

} // namespace sysy
