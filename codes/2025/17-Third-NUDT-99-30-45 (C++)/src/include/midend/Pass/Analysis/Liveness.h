#pragma once

#include "IR.h"      // 包含 IR 定义
#include "Pass.h"    // 包含 Pass 框架
#include <algorithm> // for std::set_union, std::set_difference
#include <map>
#include <set>
#include <vector>

namespace sysy {

// 前向声明
class Function;
class BasicBlock;
class Value;
class Instruction;

// 活跃变量分析结果类
// 它将包含 LiveIn 和 LiveOut 集合
class LivenessAnalysisResult : public AnalysisResultBase {
public:
  LivenessAnalysisResult(Function *F) : AssociatedFunction(F) {}
  
  // 获取给定基本块的 LiveIn 集合
  const std::set<Value *> *getLiveIn(BasicBlock *BB) const;

  // 获取给定基本块的 LiveOut 集合
  const std::set<Value *> *getLiveOut(BasicBlock *BB) const;

  // 暴露内部数据结构，如果需要更直接的访问
  const std::map<BasicBlock *, std::set<Value *>> &getLiveInSets() const { return liveInSets; }
  const std::map<BasicBlock *, std::set<Value *>> &getLiveOutSets() const { return liveOutSets; }

  // 核心计算方法，由 LivenessAnalysisPass 调用
  void computeLiveness(Function *F);

private:
  Function *AssociatedFunction; // 这个活跃变量分析是为哪个函数计算的
  std::map<BasicBlock *, std::set<Value *>> liveInSets;
  std::map<BasicBlock *, std::set<Value *>> liveOutSets;

  // 辅助函数：计算基本块的 Def 和 Use 集合
  // Def: 块内定义，且定义在所有使用之前的值
  // Use: 块内使用，且使用在所有定义之前的值
  void computeDefUse(BasicBlock *BB, std::set<Value *> &def, std::set<Value *> &use);
};

// 活跃变量分析遍
class LivenessAnalysisPass : public AnalysisPass {
public:
  // 唯一的 Pass ID
  static void *ID; // LLVM 风格的唯一 ID

  LivenessAnalysisPass() : AnalysisPass("LivenessAnalysis", Pass::Granularity::Function) {}
    
  // 实现 getPassID
  void *getPassID() const override { return &ID; }

  // 运行分析并返回结果。现在接受 AnalysisManager& AM 参数
  bool runOnFunction(Function *F, AnalysisManager &AM) override;

  // 获取分析结果的指针。
  // 注意：AnalysisManager 将会调用此方法来获取结果并进行缓存。
  std::unique_ptr<AnalysisResultBase> getResult() override;

private:
  // 存储当前分析计算出的 LivenessAnalysisResult 实例
  // runOnFunction 每次调用都会创建新的 LivenessAnalysisResult 对象
  std::unique_ptr<LivenessAnalysisResult> CurrentLivenessResult;
};

} // namespace sysy