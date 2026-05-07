#pragma once

#include "Pass.h"
#include "Loop.h" 
#include "LoopCharacteristics.h"
#include "AliasAnalysis.h"
#include "SideEffectAnalysis.h"
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace sysy {

/**
 * @brief 依赖类型枚举 - 只考虑真正影响并行性的依赖
 * 
 * 依赖类型分析说明：
 * - TRUE_DEPENDENCE (RAW): 真依赖，必须保持原始执行顺序，是最关键的依赖
 * - ANTI_DEPENDENCE (WAR): 反依赖，影响指令重排序，可通过寄存器重命名等技术缓解
 * - OUTPUT_DEPENDENCE (WAW): 输出依赖，相对较少但需要考虑，可通过变量私有化解决
 * 
 */
enum class DependenceType {
  TRUE_DEPENDENCE,    // 真依赖 (RAW) - 读后写流依赖，最重要的依赖类型
  ANTI_DEPENDENCE,    // 反依赖 (WAR) - 写后读反向依赖，影响指令重排序  
  OUTPUT_DEPENDENCE   // 输出依赖 (WAW) - 写后写，相对较少但需要考虑
};

/**
 * @brief 依赖向量 - 表示两个内存访问之间的迭代距离
 * 例如：a[i] 和 a[i+1] 之间的依赖向量是 [1]
 *       a[i][j] 和 a[i+1][j-2] 之间的依赖向量是 [1,-2]
 */
struct DependenceVector {
  std::vector<int> distances;    // 每个循环层次的依赖距离
  bool isConstant;               // 是否为常量距离
  bool isKnown;                  // 是否已知距离
  
  DependenceVector(size_t loopDepth) : distances(loopDepth, 0), isConstant(false), isKnown(false) {}
  
  // 检查是否为循环无关依赖
  bool isLoopIndependent() const {
    for (int dist : distances) {
      if (dist != 0) return false;
    }
    return true;
  }
  
  // 获取词典序方向向量
  std::vector<int> getDirectionVector() const;
  
  // 检查是否可以通过向量化处理
  bool isVectorizationSafe() const;
};

/**
 * @brief 精确依赖关系 - 包含依赖向量的详细依赖信息
 */
struct PreciseDependence {
  Instruction* source;
  Instruction* sink;
  DependenceType type;
  DependenceVector dependenceVector;
  Value* memoryLocation;
  
  // 并行化相关
  bool allowsParallelization;    // 是否允许并行化
  bool requiresSynchronization;  // 是否需要同步
  bool isReductionDependence;    // 是否为归约依赖
  
  PreciseDependence(size_t loopDepth) : dependenceVector(loopDepth), 
    allowsParallelization(true), requiresSynchronization(false), isReductionDependence(false) {}
};

/**
 * @brief 向量化分析信息 - 暂时搁置，保留接口
 */
struct VectorizationAnalysis {
  bool isVectorizable;                        // 固定为false，暂不支持
  int suggestedVectorWidth;                   // 固定为1
  std::vector<std::string> preventingFactors; // 阻止向量化的因素
  
  VectorizationAnalysis() : isVectorizable(false), suggestedVectorWidth(1) {
    preventingFactors.push_back("Vectorization temporarily disabled");
  }
};

/**
 * @brief 并行化分析信息
 */
struct ParallelizationAnalysis {
  bool isParallelizable;                     // 是否可并行化
  int suggestedThreadCount;                  // 建议的线程数
  std::vector<std::string> preventingFactors; // 阻止并行化的因素
  
  // 并行化模式
  enum ParallelizationType { 
    NONE,                    // 不可并行化
    EMBARRASSINGLY_PARALLEL, // 完全并行
    REDUCTION_PARALLEL,      // 归约并行
    PIPELINE_PARALLEL,       // 流水线并行
    CONDITIONAL_PARALLEL     // 条件并行
  } parallelType;
  
  // 负载均衡
  bool hasLoadBalance;                       // 是否有良好的负载均衡
  bool isDynamicLoadBalanced;                // 是否需要动态负载均衡
  double workComplexity;                     // 工作复杂度估计
  
  // 同步需求
  bool requiresReduction;                    // 是否需要归约操作
  bool requiresBarrier;                      // 是否需要屏障同步
  std::set<Value*> sharedVariables;         // 共享变量
  std::set<Value*> reductionVariables;      // 归约变量
  std::set<Value*> privatizableVariables;   // 可私有化变量
  
  // 内存访问模式
  bool hasMemoryConflicts;                   // 是否有内存冲突
  bool hasReadOnlyAccess;                    // 是否只有只读访问
  bool hasIndependentAccess;                 // 是否有独立的内存访问
  
  // 并行化收益评估
  double parallelizationBenefit;             // 并行化收益估计 (0-1)
  size_t communicationCost;                  // 通信开销估计
  size_t synchronizationCost;                // 同步开销估计
  
  ParallelizationAnalysis() : isParallelizable(false), suggestedThreadCount(1), parallelType(NONE),
    hasLoadBalance(true), isDynamicLoadBalanced(false), workComplexity(0.0), requiresReduction(false), 
    requiresBarrier(false), hasMemoryConflicts(false), hasReadOnlyAccess(false), hasIndependentAccess(false),
    parallelizationBenefit(0.0), communicationCost(0), synchronizationCost(0) {}
};

/**
 * @brief 循环向量化/并行化分析结果
 */
class LoopVectorizationResult : public AnalysisResultBase {
private:
  Function* AssociatedFunction;
  std::map<Loop*, VectorizationAnalysis> VectorizationMap;
  std::map<Loop*, ParallelizationAnalysis> ParallelizationMap;
  std::map<Loop*, std::vector<PreciseDependence>> DependenceMap;

public:
  LoopVectorizationResult(Function* F) : AssociatedFunction(F) {}
  ~LoopVectorizationResult() override = default;

  // 基础接口
  void addVectorizationAnalysis(Loop* loop, VectorizationAnalysis analysis) {
    VectorizationMap[loop] = std::move(analysis);
  }
  
  void addParallelizationAnalysis(Loop* loop, ParallelizationAnalysis analysis) {
    ParallelizationMap[loop] = std::move(analysis);
  }
  
  void addDependenceAnalysis(Loop* loop, std::vector<PreciseDependence> dependences) {
    DependenceMap[loop] = std::move(dependences);
  }

  // 查询接口
  const VectorizationAnalysis* getVectorizationAnalysis(Loop* loop) const {
    auto it = VectorizationMap.find(loop);
    return it != VectorizationMap.end() ? &it->second : nullptr;
  }
  
  const ParallelizationAnalysis* getParallelizationAnalysis(Loop* loop) const {
    auto it = ParallelizationMap.find(loop);
    return it != ParallelizationMap.end() ? &it->second : nullptr;
  }
  
  const std::vector<PreciseDependence>* getPreciseDependences(Loop* loop) const {
    auto it = DependenceMap.find(loop);
    return it != DependenceMap.end() ? &it->second : nullptr;
  }

  // 统计接口
  size_t getVectorizableLoopCount() const;
  size_t getParallelizableLoopCount() const;
  
  // 优化建议
  std::vector<Loop*> getVectorizationCandidates() const;
  std::vector<Loop*> getParallelizationCandidates() const;
  
  // 打印分析结果
  void print() const;
};

/**
 * @brief 循环向量化/并行化分析遍
 * 在循环规范化后执行，进行精确的依赖向量分析和向量化/并行化可行性评估
 * 专注于并行化分析，向量化功能暂时搁置
 */
class LoopVectorizationPass : public AnalysisPass {
public:
  // 唯一的 Pass ID
  static void *ID;
  
  LoopVectorizationPass() : AnalysisPass("LoopVectorization", Pass::Granularity::Function) {}

  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 核心运行方法
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  
  // 获取分析结果
  std::unique_ptr<AnalysisResultBase> getResult() override { return std::move(CurrentResult); }

private:
  std::unique_ptr<LoopVectorizationResult> CurrentResult;
  
  // ========== 主要分析方法 ==========
  void analyzeLoop(Loop* loop, LoopCharacteristics* characteristics, 
                   AliasAnalysisResult* aliasAnalysis, SideEffectAnalysisResult* sideEffectAnalysis);
  
  // ========== 依赖向量分析 ==========
  std::vector<PreciseDependence> computeDependenceVectors(Loop* loop, AliasAnalysisResult* aliasAnalysis);
  DependenceVector computeAccessDependence(Instruction* inst1, Instruction* inst2, Loop* loop);
  bool areAccessesAffinelyRelated(Value* ptr1, Value* ptr2, Loop* loop);
  
  // ========== 向量化分析 (暂时搁置) ==========
  VectorizationAnalysis analyzeVectorizability(Loop* loop, const std::vector<PreciseDependence>& dependences,
                                              LoopCharacteristics* characteristics);
  
  // ========== 并行化分析 ==========
  ParallelizationAnalysis analyzeParallelizability(Loop* loop, const std::vector<PreciseDependence>& dependences,
                                                  LoopCharacteristics* characteristics);
  bool checkParallelizationLegality(Loop* loop, const std::vector<PreciseDependence>& dependences);
  int estimateOptimalThreadCount(Loop* loop, LoopCharacteristics* characteristics);
  ParallelizationAnalysis::ParallelizationType determineParallelizationType(Loop* loop, 
                                                                           const std::vector<PreciseDependence>& dependences);
  
  // ========== 并行化专用分析方法 ==========
  void analyzeReductionPatterns(Loop* loop, ParallelizationAnalysis* analysis);
  void analyzeMemoryAccessPatterns(Loop* loop, ParallelizationAnalysis* analysis, AliasAnalysisResult* aliasAnalysis);
  void estimateParallelizationBenefit(Loop* loop, ParallelizationAnalysis* analysis, LoopCharacteristics* characteristics);
  void identifyPrivatizableVariables(Loop* loop, ParallelizationAnalysis* analysis);
  void analyzeSynchronizationNeeds(Loop* loop, ParallelizationAnalysis* analysis, const std::vector<PreciseDependence>& dependences);
  
  // ========== 辅助方法 ==========
  std::vector<int> extractInductionCoefficients(Value* ptr, Loop* loop);
  bool isConstantStride(Value* ptr, Loop* loop, int& stride);
  bool isIndependentMemoryAccess(Value* ptr1, Value* ptr2, Loop* loop);
  double estimateWorkComplexity(Loop* loop);
  bool hasReductionPattern(Value* var, Loop* loop);
};

} // namespace sysy
