#include "Liveness.h"
#include <algorithm> // For std::set_union, std::set_difference
#include <iostream>
#include <queue> // Potentially for worklist, though not strictly needed for the iterative approach below
#include <set>   // For std::set

namespace sysy {

// 初始化静态 ID
void *LivenessAnalysisPass::ID = (void *)&LivenessAnalysisPass::ID;
// ==============================================================
// LivenessAnalysisResult 结果类的实现
// ==============================================================

const std::set<Value *> *LivenessAnalysisResult::getLiveIn(BasicBlock *BB) const {
  auto it = liveInSets.find(BB);
  if (it != liveInSets.end()) {
    return &(it->second);
  }
  // 返回一个空集合，表示未找到或不存在
  static const std::set<Value *> emptySet;
  return &emptySet;
}

const std::set<Value *> *LivenessAnalysisResult::getLiveOut(BasicBlock *BB) const {
  auto it = liveOutSets.find(BB);
  if (it != liveOutSets.end()) {
    return &(it->second);
  }
  static const std::set<Value *> emptySet;
  return &emptySet;
}

void LivenessAnalysisResult::computeDefUse(BasicBlock *BB, std::set<Value *> &def, std::set<Value *> &use) {
  def.clear(); // 将持有在 BB 中定义的值
  use.clear(); // 将持有在 BB 中使用但在其定义之前的值

  // 临时集合，用于跟踪当前基本块中已经定义过的变量
  std::set<Value *> defined_in_block_so_far;

  // 按照指令在块中的顺序遍历
  for (const auto &inst_ptr : BB->getInstructions()) {
    Instruction *inst = inst_ptr.get();

    // 1. 处理指令的操作数 (Use) - 在定义之前的使用
    for (const auto &use_ptr : inst->getOperands()) { // 修正迭代器类型
      Value *operand = use_ptr->getValue();           // 从 shared_ptr<Use> 获取 Value*

      // 过滤掉常量和全局变量，因为它们通常不被视为活跃变量
      ConstantValue *constValue = dynamic_cast<ConstantValue *>(operand);
      GlobalValue *globalValue = dynamic_cast<GlobalValue *>(operand);
      if (constValue || globalValue) {
        continue; // 跳过常量和全局变量
      }

      // 如果操作数是一个变量（Instruction 或 Argument），并且它在此基本块的当前点之前尚未被定义
      if (defined_in_block_so_far.find(operand) == defined_in_block_so_far.end()) {
        use.insert(operand);
      }
    }

    // 2. 处理指令自身产生的定义 (Def)
    if (inst->isDefine()) { // 使用 isDefine() 方法
      // 指令自身定义了一个值。将其添加到块的 def 集合，
      // 并添加到当前块中已定义的值的临时集合。
      def.insert(inst); // inst 本身就是被定义的值（例如，虚拟寄存器）
      defined_in_block_so_far.insert(inst);
    }
  }
}

void LivenessAnalysisResult::computeLiveness(Function *F) {
  // 每次计算前清空旧结果
  liveInSets.clear();  // 直接清空 map，不再使用 F 作为键
  liveOutSets.clear(); // 直接清空 map

  // 初始化所有基本块的 LiveIn 和 LiveOut 集合为空
  for (const auto &bb_ptr : F->getBasicBlocks()) {
    BasicBlock *bb = bb_ptr.get();
    liveInSets[bb] = {};  // 直接以 bb 为键
    liveOutSets[bb] = {}; // 直接以 bb 为键
  }

  bool changed = true;
  while (changed) {
    changed = false;

    // TODO : 目前为逆序遍历基本块，考虑反向拓扑序遍历基本块

    // 逆序遍历基本块
    // std::list<std::unique_ptr<BasicBlock>> basicBlocks(F->getBasicBlocks().begin(), F->getBasicBlocks().end());
    // std::reverse(basicBlocks.begin(), basicBlocks.end());
    // 然后遍历 basicBlocks
    // 创建一个 BasicBlock* 的列表来存储指针，避免拷贝 unique_ptr
    // Option 1: Using std::vector<BasicBlock*> (preferred for performance with reverse)
    std::vector<BasicBlock*> basicBlocksPointers;
    for (const auto& bb_ptr : F->getBasicBlocks()) {
        basicBlocksPointers.push_back(bb_ptr.get());
    }
    std::reverse(basicBlocksPointers.begin(), basicBlocksPointers.end());

    for (auto bb_iter = basicBlocksPointers.begin(); bb_iter != basicBlocksPointers.end(); ++bb_iter) {
      BasicBlock *bb = *bb_iter; // 获取 BasicBlock 指针
      if (!bb)
        continue; // 避免空指针

      std::set<Value *> oldLiveIn = liveInSets[bb];
      std::set<Value *> oldLiveOut = liveOutSets[bb];

      // 1. 计算 LiveOut(BB) = Union(LiveIn(Succ) for Succ in Successors(BB))
      std::set<Value *> newLiveOut;
      for (BasicBlock *succ : bb->getSuccessors()) {
        const std::set<Value *> *succLiveIn = getLiveIn(succ); // 获取后继的 LiveIn
        if (succLiveIn) {
          newLiveOut.insert(succLiveIn->begin(), succLiveIn->end());
        }
      }
      liveOutSets[bb] = newLiveOut;

      // 2. 计算 LiveIn(BB) = Use(BB) Union (LiveOut(BB) - Def(BB))
      std::set<Value *> defSet, useSet;
      computeDefUse(bb, defSet, useSet); // 计算当前块的 Def 和 Use

      std::set<Value *> liveOutMinusDef;
      std::set_difference(newLiveOut.begin(), newLiveOut.end(), defSet.begin(), defSet.end(),
                          std::inserter(liveOutMinusDef, liveOutMinusDef.begin()));

      std::set<Value *> newLiveIn = useSet;
      newLiveIn.insert(liveOutMinusDef.begin(), liveOutMinusDef.end());
      liveInSets[bb] = newLiveIn;

      // 检查是否发生变化
      if (oldLiveIn != newLiveIn || oldLiveOut != newLiveOut) {
        changed = true;
      }
    }
  }
}

// ==============================================================
// LivenessAnalysisPass 的实现
// ==============================================================

bool LivenessAnalysisPass::runOnFunction(Function *F, AnalysisManager &AM) {
  // 每次运行创建一个新的 LivenessAnalysisResult 对象来存储结果
  CurrentLivenessResult = std::make_unique<LivenessAnalysisResult>(F);

  // 调用 LivenessAnalysisResult 内部的方法来计算分析结果
  CurrentLivenessResult->computeLiveness(F);

  // 分析遍通常不修改 IR，所以返回 false
  return false;
}

std::unique_ptr<AnalysisResultBase> LivenessAnalysisPass::getResult() {
  // 返回计算好的 LivenessAnalysisResult 实例，所有权转移给 AnalysisManager
  return std::move(CurrentLivenessResult);
}

} // namespace sysy