#include "DCE.h"
#include "SysYIROptUtils.h"
#include "SideEffectAnalysis.h"
#include <cassert>          
#include <iostream>         
#include <set>              

namespace sysy {

// DCE 遍的静态 ID
void *DCE::ID = (void *)&DCE::ID;

// ======================================================================
// DCEContext 类的实现
// 封装了 DCE 遍的核心逻辑和状态，确保每次函数优化运行时状态独立
// ======================================================================

// DCEContext 的 run 方法实现
void DCEContext::run(Function *func, AnalysisManager *AM, bool &changed) {
  // 获取别名分析结果
  if (AM) {
    aliasAnalysis = AM->getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(func);
    // 获取副作用分析结果（Module级别）
    sideEffectAnalysis = AM->getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();
    
    if (DEBUG) {
      if (aliasAnalysis) {
        std::cout << "DCE: Using alias analysis results" << std::endl;
      }
      if (sideEffectAnalysis) {
        std::cout << "DCE: Using side effect analysis results" << std::endl;
      }
    }
  }
  
  // 清空活跃指令集合，确保每次运行都是新的状态
  alive_insts.clear();

  // 第一次遍历：扫描所有指令，识别"天然活跃"的指令并将其及其依赖标记为活跃
  // 使用 func->getBasicBlocks() 获取基本块列表，保留用户风格
  auto basicBlocks = func->getBasicBlocks();
  for (auto &basicBlock : basicBlocks) {
    // 确保基本块有效
    if (!basicBlock)
      continue;
    // 使用 basicBlock->getInstructions() 获取指令列表，保留用户风格
    for (auto &inst : basicBlock->getInstructions()) {
      // 确保指令有效
      if (!inst)
        continue;
      // 调用 DCEContext 自身的 isAlive 和 addAlive 方法
      if (isAlive(inst.get())) {
        addAlive(inst.get());
      }
    }
  }

  // 第二次遍历：删除所有未被标记为活跃的指令。
  for (auto &basicBlock : basicBlocks) {
    if (!basicBlock)
      continue;
    // 使用传统的迭代器循环，并手动管理迭代器，
    // 以便在删除元素后正确前进。保留用户风格
    for (auto instIter = basicBlock->getInstructions().begin(); instIter != basicBlock->getInstructions().end();) {
      auto &inst = *instIter;
      Instruction *currentInst = inst.get();
      // 如果指令不在活跃集合中，则删除它。
      // 分支和返回指令由 isAlive 处理，并会被保留。
      if (alive_insts.count(currentInst) == 0) {
        instIter = SysYIROptUtils::usedelete(instIter); // 删除后返回下一个迭代器
        changed = true; // 标记 IR 已被修改
      } else {
        ++instIter; // 指令活跃，移动到下一个
      }
    }
  }
  changed |= SysYIROptUtils::eliminateRedundantPhisInFunction(func); // 如果有活跃指令，则标记为已更改
}

// 判断指令是否是"天然活跃"的实现
// 只有具有副作用的指令（如存储、函数调用、原子操作）
// 和控制流指令（如分支、返回）是天然活跃的。
bool DCEContext::isAlive(Instruction *inst) {
  // 终止指令 (BranchInst, ReturnInst) 必须是活跃的，因为它控制了程序的执行流程
  if (inst->isBranch() || inst->isReturn()) {
    return true;
  }
  
  // 使用副作用分析来判断指令是否有副作用
  if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(inst)) {
    return true;
  }
  
  // 特殊处理Store指令：使用别名分析进行更精确的判断
  if (inst->isStore()) {
    auto* storeInst = static_cast<StoreInst*>(inst);
    return mayHaveSideEffect(storeInst);
  }
  
  // 特殊处理Memset指令：总是保留（因为它修改内存）
  if (inst->isMemset()) {
    return true;
  }
  
  // 函数调用指令：总是保留（可能有未知副作用）
  if (inst->isCall()) {
    return true;
  }
  
  // 其他指令（算术、逻辑、Load等）：无副作用，可以删除
  return false;
}

// 检查Store指令是否可能有副作用（通过别名分析）
bool DCEContext::mayHaveSideEffect(StoreInst* store) {
  if (!aliasAnalysis) {
    // 没有别名分析结果时，保守地认为所有store都有副作用
    return true;
  }
  
  Value* storePtr = store->getPointer();
  
  // 如果是对本地数组的存储且访问模式是常量，可能可以安全删除
  if (aliasAnalysis->isLocalArray(storePtr)) {
    // 检查是否有其他指令可能读取这个位置
    // 这里需要更复杂的活性分析，暂时保守处理
    return true; // 保守地保留所有本地数组的存储
  }
  
  // 对全局变量、函数参数等的存储总是有副作用
  return true;
}

// 递归地将活跃指令及其依赖加入到 alive_insts 集合中
void DCEContext::addAlive(Instruction *inst) {
  // 如果指令已经存在于活跃集合中，则无需重复处理
  if (alive_insts.count(inst) > 0) {
    return;
  }
  // 将当前指令标记为活跃
  alive_insts.insert(inst);
  // 遍历当前指令的所有操作数
  // 保留用户提供的 getOperands() 和 getValue()
  for (auto operand : inst->getOperands()) {
    // 如果操作数是一个指令（即它是一个值的定义），
    // 并且它还没有被标记为活跃
    if (auto opInst = dynamic_cast<Instruction *>(operand->getValue())) {
      addAlive(opInst); // 递归地将操作数指令标记为活跃
    }
  }
}

// ======================================================================
// DCE Pass 类的实现
// 主要负责与 PassManager 交互，创建 DCEContext 实例并运行优化
// ======================================================================

// DCE 遍的 runOnFunction 方法实现
bool DCE::runOnFunction(Function *func, AnalysisManager &AM) {
  DCEContext ctx;
  bool changed = false;
  ctx.run(func, &AM, changed); // 运行 DCE 优化

  // 如果 IR 被修改，则使相关的分析结果失效
  if (changed) {
    // DCE 会删除指令，这会影响数据流分析，尤其是活跃性分析。
    // 如果删除导致基本块变空，也可能间接影响 CFG 和支配树。
    // AM.invalidateAnalysis(&LivenessAnalysisPass::ID, func); // 活跃性分析失效
    // AM.invalidateAnalysis(&DominatorTreeAnalysisPass::ID, func); // 支配树分析可能失效
    // 其他所有依赖于数据流或 IR 结构的分析都可能失效。
  }
  return changed;
}

// 声明DCE遍的分析依赖和失效信息
void DCE::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // DCE依赖别名分析来更精确地判断Store指令的副作用
  analysisDependencies.insert(&SysYAliasAnalysisPass::ID);
  
  // DCE依赖副作用分析来判断指令是否有副作用
  analysisDependencies.insert(&SysYSideEffectAnalysisPass::ID);

  // DCE会删除指令，这会影响许多分析结果。
  // 至少，它会影响活跃性分析、支配树、控制流图（如果删除导致基本块为空并被合并）。
  // 假设存在LivenessAnalysisPass和DominatorTreeAnalysisPass
  // analysisInvalidations.insert(&LivenessAnalysisPass::ID);
  // analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID);
  // 任何改变IR结构的优化，都可能导致通用分析（如活跃性、支配树、循环信息）失效。
  // 最保守的做法是使所有函数粒度的分析失效，或者只声明你明确知道会受影响的分析。
  // 考虑到这个DCE仅删除指令，如果它不删除基本块，CFG可能不变，但数据流分析会失效。
  // 对于更激进的DCE（如ADCE），CFG也会改变。
  // 这里我们假设它主要影响数据流分析，并且可能间接影响CFG相关分析。
  // 如果有SideEffectInfo，它也可能被修改，但通常SideEffectInfo是静态的，不因DCE而变。
}

} // namespace sysy
