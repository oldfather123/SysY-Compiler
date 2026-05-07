#pragma once

#include "Pass.h"
#include "IR.h"
#include "LoopCharacteristics.h"
#include "Loop.h"
#include "Dom.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace sysy {

// 前向声明
class LoopCharacteristicsResult;
class LoopAnalysisResult;

/**
 * @brief 强度削弱候选项信息
 * 记录一个可以进行强度削弱的表达式信息
 */
struct StrengthReductionCandidate {
  enum OpType {
    MULTIPLY,    // 乘法: iv * const
    DIVIDE,      // 除法: iv / 2^n (转换为右移)
    DIVIDE_CONST, // 除法: iv / const (使用mulh指令优化)
    REMAINDER    // 取模: iv % 2^n (转换为位与)
  };
  
  enum DivisionStrategy {
    SIMPLE_SHIFT,     // 简单右移（仅适用于无符号或非负数）
    SIGNED_CORRECTION, // 有符号除法修正: (x + (x >> 31) & mask) >> k
    MULH_OPTIMIZATION  // 使用mulh指令优化任意常数除法
  };
  
  Instruction* originalInst;      // 原始指令 (如 i*4, i/8, i%16)
  Value* inductionVar;            // 归纳变量 (如 i)
  OpType operationType;           // 操作类型
  DivisionStrategy divStrategy;   // 除法策略（仅用于除法）
  int multiplier;                 // 乘数/除数/模数 (如 4, 8, 16)
  int shiftAmount;                // 位移量 (对于2的幂)
  int offset;                     // 偏移量 (如常数项)
  BasicBlock* containingBlock;    // 所在基本块
  Loop* containingLoop;           // 所在循环
  bool hasNegativeValues;         // 归纳变量是否可能为负数
  
  // 强度削弱后的新变量
  PhiInst* newPhi = nullptr;      // 新的 phi 指令
  Value* newInductionVar = nullptr; // 新的归纳变量
  
  StrengthReductionCandidate(Instruction* inst, Value* iv, OpType opType, int value, int off, 
                           BasicBlock* bb, Loop* loop)
    : originalInst(inst), inductionVar(iv), operationType(opType), 
      divStrategy(SIMPLE_SHIFT), multiplier(value), offset(off), 
      containingBlock(bb), containingLoop(loop), hasNegativeValues(false) {
    
    // 计算位移量（用于除法和取模的强度削弱）
    if (opType == DIVIDE || opType == REMAINDER) {
      shiftAmount = 0;
      int temp = value;
      while (temp > 1) {
        temp >>= 1;
        shiftAmount++;
      }
    } else {
      shiftAmount = 0;
    }
  }
};

/**
 * @brief 强度削弱上下文类
 * 封装强度削弱优化的核心逻辑和状态
 */
class StrengthReductionContext {
public:
  StrengthReductionContext(IRBuilder* builder) : builder(builder) {}
  
  /**
   * 运行强度削弱优化
   * @param F 目标函数
   * @param AM 分析管理器
   * @return 是否修改了IR
   */
  bool run(Function* F, AnalysisManager& AM);

private:
  IRBuilder* builder;
  
  // 分析结果缓存
  LoopAnalysisResult* loopAnalysis = nullptr;
  LoopCharacteristicsResult* loopCharacteristics = nullptr;
  DominatorTree* dominatorTree = nullptr;
  
  // 候选项存储
  std::vector<std::unique_ptr<StrengthReductionCandidate>> candidates;
  std::unordered_map<Loop*, std::vector<StrengthReductionCandidate*>> loopToCandidates;
  
  // ========== 核心分析和优化阶段 ==========
  
  /**
   * 阶段1：识别强度削弱候选项
   * 扫描所有循环中的乘法指令，找出可以优化的模式
   */
  void identifyStrengthReductionCandidates(Function* F);
  
  /**
   * 阶段2：分析候选项的优化潜力
   * 评估每个候选项的收益，过滤掉不值得优化的情况
   */
  void analyzeOptimizationPotential();
  
  /**
   * 阶段3：执行强度削弱变换
   * 对选中的候选项执行实际的强度削弱优化
   */
  bool performStrengthReduction();
  
  // ========== 辅助分析函数 ==========
  
  /**
   * 分析归纳变量是否可能取负值
   * @param ivInfo 归纳变量信息
   * @param loop 所属循环
   * @return 如果可能为负数返回true
   */
  bool analyzeInductionVariableRange(const InductionVarInfo* ivInfo, Loop* loop) const;
  
  /**
   * 生成除法替换代码
   * @param candidate 优化候选项
   * @param builder IR构建器
   * @return 替换值
   */
  Value* generateDivisionReplacement(StrengthReductionCandidate* candidate, IRBuilder* builder) const;
  
  /**
   * 生成任意常数除法替换代码
   * @param candidate 优化候选项
   * @param builder IR构建器
   * @return 替换值
   */
  Value* generateConstantDivisionReplacement(StrengthReductionCandidate* candidate, IRBuilder* builder) const;
  
  /**
   * 检查指令是否为强度削弱候选项
   * @param inst 要检查的指令
   * @param loop 所在循环
   * @return 如果是候选项返回候选项信息，否则返回nullptr
   */
  std::unique_ptr<StrengthReductionCandidate> 
  isStrengthReductionCandidate(Instruction* inst, Loop* loop);
  
  /**
   * 检查值是否为循环的归纳变量
   * @param val 要检查的值
   * @param loop 循环
   * @param characteristics 循环特征信息
   * @return 如果是归纳变量返回归纳变量信息，否则返回nullptr
   */
  const InductionVarInfo* 
  getInductionVarInfo(Value* val, Loop* loop, const LoopCharacteristics* characteristics);
  
  /**
   * 为候选项创建新的归纳变量
   * @param candidate 候选项
   * @return 是否成功创建
   */
  bool createNewInductionVariable(StrengthReductionCandidate* candidate);
  
  /**
   * 替换原始指令的所有使用
   * @param candidate 候选项
   * @return 是否成功替换
   */
  bool replaceOriginalInstruction(StrengthReductionCandidate* candidate);
  
  /**
   * 估算优化收益
   * 计算强度削弱后的性能提升
   * @param candidate 候选项
   * @return 估算的收益分数
   */
  double estimateOptimizationBenefit(const StrengthReductionCandidate* candidate);
  
  /**
   * 检查优化的合法性
   * @param candidate 候选项
   * @return 是否可以安全地进行优化
   */
  bool isOptimizationLegal(const StrengthReductionCandidate* candidate);
  
  /**
   * 打印调试信息
   */
  void printDebugInfo();
};

/**
 * @brief 循环强度削弱优化遍
 * 将循环中的乘法运算转换为更高效的加法运算
 */
class LoopStrengthReduction : public OptimizationPass {
public:
  // 唯一的 Pass ID
  static void *ID;
  
  LoopStrengthReduction(IRBuilder* builder) 
    : OptimizationPass("LoopStrengthReduction", Granularity::Function), 
      builder(builder) {}
  
  /**
   * 在函数上运行强度削弱优化
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

private:
  IRBuilder* builder;
};

} // namespace sysy
