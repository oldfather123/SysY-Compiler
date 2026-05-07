#pragma once

#include "IR.h"
#include "Pass.h"
#include <map>
#include <set>
#include <vector>
#include <memory>

namespace sysy {

// 前向声明
class MemoryLocation;
class AliasAnalysisResult;

/**
 * @brief 别名关系类型
 * 按风险等级递增排序
 */
enum class AliasType {
  NO_ALIAS = 0,        // 确定无别名 (不同的局部数组)
  SELF_ALIAS = 1,      // 自别名 (同一数组的不同索引)
  POSSIBLE_ALIAS = 2,  // 可能有别名 (函数参数数组)
  UNKNOWN_ALIAS = 3    // 未知 (保守估计)
};

/**
 * @brief 内存位置信息
 * 描述一个内存访问的基础信息
 */
struct MemoryLocation {
  Value* basePointer;              // 基指针 (剥离GEP后的真实基址)
  Value* accessPointer;            // 访问指针 (包含索引信息)
  
  // 分类信息
  bool isLocalArray;               // 是否为局部数组
  bool isFunctionParameter;        // 是否为函数参数
  bool isGlobalArray;              // 是否为全局数组
  
  // 索引信息
  std::vector<Value*> indices;     // GEP索引列表
  bool hasConstantIndices;         // 是否为常量索引
  bool hasLoopVariableIndex;       // 是否包含循环变量
  int constantOffset;              // 常量偏移量 (仅当全部为常量时有效)
  
  // 访问模式
  bool hasReads;                   // 是否有读操作
  bool hasWrites;                  // 是否有写操作
  std::vector<Instruction*> accessInsts; // 所有访问指令
  
  MemoryLocation(Value* base, Value* access) 
    : basePointer(base), accessPointer(access), 
      isLocalArray(false), isFunctionParameter(false), isGlobalArray(false),
      hasConstantIndices(false), hasLoopVariableIndex(false), constantOffset(0),
      hasReads(false), hasWrites(false) {}
};

/**
 * @brief 别名分析结果
 * 存储一个函数的完整别名分析信息
 */
class AliasAnalysisResult : public AnalysisResultBase {
public:
  AliasAnalysisResult(Function *F) : AssociatedFunction(F) {}
  ~AliasAnalysisResult() override = default;

  // ========== 基础查询接口 ==========
  
  /**
   * 查询两个指针之间的别名关系
   */
  AliasType queryAlias(Value* ptr1, Value* ptr2) const;
  
  /**
   * 查询指针的内存位置信息
   */
  const MemoryLocation* getMemoryLocation(Value* ptr) const;
  
  /**
   * 获取所有内存位置
   */
  const std::map<Value*, std::unique_ptr<MemoryLocation>>& getAllMemoryLocations() const {
    return LocationMap;
  }
  
  // ========== 高级查询接口 ==========
  
  /**
   * 检查指针是否为局部数组
   */
  bool isLocalArray(Value* ptr) const;
  
  /**
   * 检查指针是否为函数参数数组
   */
  bool isFunctionParameter(Value* ptr) const;
  
  /**
   * 检查指针是否为全局数组
   */
  bool isGlobalArray(Value* ptr) const;
  
  /**
   * 检查指针是否使用常量索引
   */
  bool hasConstantAccess(Value* ptr) const;
  
  // ========== 统计接口 ==========
  
  /**
   * 获取各类别名类型的统计信息
   */
  struct Statistics {
    int totalQueries;
    int noAlias;
    int selfAlias;
    int possibleAlias;
    int unknownAlias;
    int localArrays;
    int functionParameters;
    int globalArrays;
    int constantAccesses;
  };
  
  Statistics getStatistics() const;
  
  /**
   * 打印别名分析结果 (调试用)
   */
  void print() const;
  void printStatics() const;
  // ========== 内部方法 ==========
  
  void addMemoryLocation(std::unique_ptr<MemoryLocation> location);
  void addAliasRelation(Value* ptr1, Value* ptr2, AliasType type);
  
  // ========== 公开数据成员 (供Pass使用) ==========
  std::map<Value*, std::unique_ptr<MemoryLocation>> LocationMap;  // 内存位置映射
  std::map<std::pair<Value*, Value*>, AliasType> AliasMap;        // 别名关系缓存

private:
  Function *AssociatedFunction;                                    // 关联的函数
  
  // 分类存储
  std::vector<Argument*> ArrayParameters;                         // 数组参数
  std::vector<AllocaInst*> LocalArrays;                          // 局部数组
  std::set<GlobalValue*> AccessedGlobals;                        // 访问的全局变量
};

/**
 * @brief SysY语言特化的别名分析Pass
 * 针对SysY语言特性优化的别名分析实现
 */
class SysYAliasAnalysisPass : public AnalysisPass {
public:
  // 唯一的 Pass ID
  static void *ID;
  // 在这里开启激进分析策略
  SysYAliasAnalysisPass() : AnalysisPass("SysYAliasAnalysis", Pass::Granularity::Function), 
                           aggressiveParameterMode(false), parameterOptimizationEnabled(false) {}

  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 核心运行方法
  bool runOnFunction(Function *F, AnalysisManager &AM) override;

  // 获取分析结果
  std::unique_ptr<AnalysisResultBase> getResult() override { return std::move(CurrentResult); }
  
  // ========== 配置接口 ==========
  
  /**
   * 启用针对SysY评测的激进优化模式
   * 在这种模式下，假设不同参数不会传入相同数组
   */
  void enableSysYTestingMode() {
    aggressiveParameterMode = true;
    parameterOptimizationEnabled = true;
  }
  
  /**
   * 使用保守的默认模式（适合通用场景）
   */
  void useConservativeMode() {
    aggressiveParameterMode = false;
    parameterOptimizationEnabled = false;
  }

private:
  std::unique_ptr<AliasAnalysisResult> CurrentResult; // 当前函数的分析结果
  
  // ========== 主要分析流程 ==========
  
  void collectMemoryAccesses(Function* F);              // 收集内存访问
  void buildAliasRelations(Function* F);                // 构建别名关系
  void optimizeForSysY(Function* F);                    // SysY特化优化
  
  // ========== 内存位置分析 ==========
  
  std::unique_ptr<MemoryLocation> createMemoryLocation(Value* ptr);
  Value* getBasePointer(Value* ptr);                    // 获取基指针
  void analyzeMemoryType(MemoryLocation* location);     // 分析内存类型
  void analyzeIndexPattern(MemoryLocation* location);   // 分析索引模式
  
  // ========== 别名关系推断 ==========
  
  AliasType analyzeAliasBetween(MemoryLocation* loc1, MemoryLocation* loc2);
  AliasType compareIndices(MemoryLocation* loc1, MemoryLocation* loc2);
  AliasType compareLocalArrays(MemoryLocation* loc1, MemoryLocation* loc2);
  AliasType compareParameters(MemoryLocation* loc1, MemoryLocation* loc2);
  AliasType compareWithGlobal(MemoryLocation* loc1, MemoryLocation* loc2);
  AliasType compareMixedTypes(MemoryLocation* loc1, MemoryLocation* loc2);
  
  // ========== SysY特化优化 ==========
  
  void applySysYConstraints(Function* F);               // 应用SysY语言约束
  void optimizeParameterAnalysis(Function* F);          // 优化参数分析
  void optimizeArrayAccessAnalysis(Function* F);        // 优化数组访问分析
  
  // ========== 配置和策略控制 ==========
  
  bool useAggressiveParameterAnalysis() const { return aggressiveParameterMode; }
  bool enableParameterOptimization() const { return parameterOptimizationEnabled; }
  void setAggressiveParameterMode(bool enable) { aggressiveParameterMode = enable; }
  void setParameterOptimizationEnabled(bool enable) { parameterOptimizationEnabled = enable; }
  
  // ========== 辅助优化方法 ==========
  
  void optimizeConstantIndexAccesses();                 // 优化常量索引访问
  void optimizeSequentialAccesses();                   // 优化顺序访问
  
  // ========== 辅助方法 ==========
  
  bool isConstantValue(Value* val);                     // 是否为常量
  bool hasLoopVariableInIndices(const std::vector<Value*>& indices, Function* F);
  int calculateConstantOffset(const std::vector<Value*>& indices);
  void printStatistics() const;                         // 打印统计信息

private:
  // ========== 配置选项 ==========
  bool aggressiveParameterMode = false;                // 激进的参数别名分析模式
  bool parameterOptimizationEnabled = false;           // 启用参数优化
};

} // namespace sysy
