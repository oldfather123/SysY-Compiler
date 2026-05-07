#pragma once

#include "IR.h"
#include "Pass.h"
#include "SysYIROptUtils.h"
#include "AliasAnalysis.h"
#include "SideEffectAnalysis.h"
#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>
#include <variant>
#include <functional>

namespace sysy {

// 定义三值格 (Three-valued Lattice) 的状态
enum class LatticeVal {
  Top,      // ⊤ (未知 / 未初始化)
  Constant, // c (常量)
  Bottom    // ⊥ (不确定 / 变化 / 未定义)
};

// 新增枚举来区分常量的实际类型
enum class ValueType {
  Integer,
  Float,
  Unknown // 用于 Top 和 Bottom 状态
};

// 用于表示 SSA 值的具体状态（包含格值和常量值）
struct SSAPValue {
  LatticeVal state;
  std::variant<int, float> constantVal; // 使用 std::variant 存储 int 或 float
  ValueType constant_type; // 记录常量是整数还是浮点数

  // 默认构造函数，初始化为 Top
  SSAPValue() : state(LatticeVal::Top), constantVal(0), constant_type(ValueType::Unknown) {}
  // 构造函数，用于创建 Bottom 状态
  SSAPValue(LatticeVal s) : state(s), constantVal(0), constant_type(ValueType::Unknown) {
    assert((s == LatticeVal::Top || s == LatticeVal::Bottom) && "SSAPValue(LatticeVal) only for Top/Bottom");
  }
  // 构造函数，用于创建 int Constant 状态
  SSAPValue(int c) : state(LatticeVal::Constant), constantVal(c), constant_type(ValueType::Integer) {}
  // 构造函数，用于创建 float Constant 状态
  SSAPValue(float c) : state(LatticeVal::Constant), constantVal(c), constant_type(ValueType::Float) {}

  // 比较操作符，用于判断状态是否改变
  bool operator==(const SSAPValue &other) const {
    if (state != other.state)
      return false;
    if (state == LatticeVal::Constant) {
        if (constant_type != other.constant_type) return false; // 类型必须匹配
        return constantVal == other.constantVal; // std::variant 会比较内部值
    }
    return true; // Top == Top, Bottom == Bottom
  }
  bool operator!=(const SSAPValue &other) const { return !(*this == other); }
};

// SCCP 上下文类，持有每个函数运行时的状态
class SCCPContext {
private:
  IRBuilder *builder; // IR 构建器，用于插入指令和创建常量
  AliasAnalysisResult *aliasAnalysis; // 别名分析结果
  SideEffectAnalysisResult *sideEffectAnalysis; // 副作用分析结果

  // 工作列表
  // 存储需要重新评估的指令
  std::queue<Instruction *> instWorkList;
  // 存储需要重新评估的控制流边 (pair: from_block, to_block)
  std::queue<std::pair<BasicBlock *, BasicBlock *>> edgeWorkList;

  // 格值映射：SSA Value 到其当前状态
  std::map<Value *, SSAPValue> valueState;
  // 可执行基本块集合
  std::unordered_set<BasicBlock *> executableBlocks;
  // 追踪已访问的CFG边，防止重复添加，使用 SysYIROptUtils::PairHash
  std::unordered_set<std::pair<BasicBlock*, BasicBlock*>, SysYIROptUtils::PairHash> visitedCFGEdges;

  // 辅助函数：格操作 Meet
  SSAPValue Meet(const SSAPValue &a, const SSAPValue &b);
  // 辅助函数：获取值的当前状态，如果不存在则默认为 Top
  SSAPValue GetValueState(Value *v);
  // 辅助函数：更新值的状态，如果状态改变，将所有用户加入指令工作列表
  void UpdateState(Value *v, SSAPValue newState);
  // 辅助函数：将边加入边工作列表，并更新可执行块
  void AddEdgeToWorkList(BasicBlock *fromBB, BasicBlock *toBB);
  // 辅助函数：标记一个块为可执行
  void MarkBlockExecutable(BasicBlock* block);

  // 辅助函数：对二元操作进行常量折叠
  SSAPValue ComputeConstant(BinaryInst *binaryinst, SSAPValue lhsVal, SSAPValue rhsVal);
  // 辅助函数：对一元操作进行常量折叠
  SSAPValue ComputeConstant(UnaryInst *unaryInst, SSAPValue operandVal);
  // 辅助函数：检查是否为已知的纯函数
  bool isKnownPureFunction(const std::string &funcName) const;
  // 辅助函数：计算纯函数的常量结果
  SSAPValue computePureFunctionResult(CallInst *call, const std::vector<SSAPValue> &argValues);
  // 辅助函数：查找存储到指定位置的常量值
  SSAPValue findStoredConstantValue(Value *ptr, BasicBlock *currentBB);
  // 辅助函数：动态检查数组访问是否为常量索引（考虑SCCP状态）
  bool hasRuntimeConstantAccess(Value *ptr);

  // 主要优化阶段
  // 阶段1: 常量传播与折叠
  bool PropagateConstants(Function *func);
  // 阶段2: 控制流简化
  bool SimplifyControlFlow(Function *func);

  // 辅助函数：处理单条指令
  void ProcessInstruction(Instruction *inst);
  // 辅助函数：处理单条控制流边
  void ProcessEdge(const std::pair<BasicBlock *, BasicBlock *> &edge);

  // 控制流简化辅助函数
  // 查找所有可达的基本块 (基于常量条件)
  std::unordered_set<BasicBlock *> FindReachableBlocks(Function *func);
  // 移除死块
  void RemoveDeadBlock(BasicBlock *bb, Function *func);
  // 简化分支（将条件分支替换为无条件分支）
  void SimplifyBranch(CondBrInst*brInst, bool condVal); // 保持 BranchInst
  // 更新前驱块的终结指令（当一个后继块被移除时）
  void UpdateTerminator(BasicBlock *predBB, BasicBlock *removedSucc);
  // 移除 Phi 节点的入边（当其前驱块被移除时）
  void RemovePhiIncoming(BasicBlock *phiParentBB, BasicBlock *removedPred);

public:
  SCCPContext(IRBuilder *builder) : builder(builder), aliasAnalysis(nullptr), sideEffectAnalysis(nullptr) {}
  
  // 设置别名分析结果
  void setAliasAnalysis(AliasAnalysisResult *aa) { aliasAnalysis = aa; }
  
  // 设置副作用分析结果
  void setSideEffectAnalysis(SideEffectAnalysisResult *sea) { sideEffectAnalysis = sea; }

  // 运行 SCCP 优化
  void run(Function *func, AnalysisManager &AM);
};

// SCCP 优化遍类，继承自 OptimizationPass
class SCCP : public OptimizationPass {
private:
  IRBuilder *builder; // IR 构建器，作为 Pass 的成员，传入 Context

public:
  SCCP(IRBuilder *builder) : OptimizationPass("SCCP", Granularity::Function), builder(builder) {}
  static void *ID;
  bool runOnFunction(Function *F, AnalysisManager &AM) override;
  void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const override;
  void *getPassID() const override { return &ID; }
};

} // namespace sysy