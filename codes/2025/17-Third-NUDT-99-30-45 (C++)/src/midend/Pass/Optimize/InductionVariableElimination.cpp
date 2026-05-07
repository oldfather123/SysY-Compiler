#include "InductionVariableElimination.h"
#include "LoopCharacteristics.h"
#include "Loop.h"
#include "Dom.h"
#include "SideEffectAnalysis.h"
#include "AliasAnalysis.h"
#include "SysYIROptUtils.h"
#include <iostream>
#include <algorithm>

// 使用全局调试开关
extern int DEBUG;

namespace sysy {

// 定义 Pass 的唯一 ID
void *InductionVariableElimination::ID = (void *)&InductionVariableElimination::ID;

bool InductionVariableElimination::runOnFunction(Function* F, AnalysisManager& AM) {
  if (F->getBasicBlocks().empty()) {
    return false; // 空函数
  }

  if (DEBUG) {
    std::cout << "Running InductionVariableElimination on function: " << F->getName() << std::endl;
  }

  // 创建优化上下文并运行
  InductionVariableEliminationContext context;
  bool modified = context.run(F, AM);

  if (DEBUG) {
    std::cout << "InductionVariableElimination " << (modified ? "modified" : "did not modify") 
              << " function: " << F->getName() << std::endl;
  }

  return modified;
}

void InductionVariableElimination::getAnalysisUsage(std::set<void*>& analysisDependencies, 
                                                   std::set<void*>& analysisInvalidations) const {
  // 依赖的分析
  analysisDependencies.insert(&LoopAnalysisPass::ID);
  analysisDependencies.insert(&LoopCharacteristicsPass::ID);
  analysisDependencies.insert(&DominatorTreeAnalysisPass::ID);
  analysisDependencies.insert(&SysYSideEffectAnalysisPass::ID);
  analysisDependencies.insert(&SysYAliasAnalysisPass::ID);
  
  // 会使失效的分析（归纳变量消除会修改IR结构）
  analysisInvalidations.insert(&LoopCharacteristicsPass::ID);
  // 注意：支配树分析通常不会因为归纳变量消除而失效，因为我们不改变控制流
}

// ========== InductionVariableEliminationContext 实现 ==========

bool InductionVariableEliminationContext::run(Function* F, AnalysisManager& AM) {
  if (DEBUG) {
    std::cout << "  Starting induction variable elimination analysis..." << std::endl;
  }

  // 获取必要的分析结果
  loopAnalysis = AM.getAnalysisResult<LoopAnalysisResult, LoopAnalysisPass>(F);
  if (!loopAnalysis || !loopAnalysis->hasLoops()) {
    if (DEBUG) {
      std::cout << "  No loops found, skipping induction variable elimination" << std::endl;
    }
    return false;
  }

  loopCharacteristics = AM.getAnalysisResult<LoopCharacteristicsResult, LoopCharacteristicsPass>(F);
  if (!loopCharacteristics) {
    if (DEBUG) {
      std::cout << "  LoopCharacteristics analysis not available" << std::endl;
    }
    return false;
  }

  dominatorTree = AM.getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(F);
  if (!dominatorTree) {
    if (DEBUG) {
      std::cout << "  DominatorTree analysis not available" << std::endl;
    }
    return false;
  }

  sideEffectAnalysis = AM.getAnalysisResult<SideEffectAnalysisResult, SysYSideEffectAnalysisPass>();
  if (!sideEffectAnalysis) {
    if (DEBUG) {
      std::cout << "  SideEffectAnalysis not available, using conservative approach" << std::endl;
    }
    // 可以继续执行，但会使用更保守的策略
  } else {
    if (DEBUG) {
      std::cout << "  Using SideEffectAnalysis for safety checks" << std::endl;
    }
  }

  aliasAnalysis = AM.getAnalysisResult<AliasAnalysisResult, SysYAliasAnalysisPass>(F);
  if (!aliasAnalysis) {
    if (DEBUG) {
      std::cout << "  AliasAnalysis not available, using conservative approach" << std::endl;
    }
    // 可以继续执行，但会使用更保守的策略
  } else {
    if (DEBUG) {
      std::cout << "  Using AliasAnalysis for memory safety checks" << std::endl;
    }
  }

  // 执行三个阶段的优化
  
  // 阶段1：识别死归纳变量
  identifyDeadInductionVariables(F);
  
  if (deadIVs.empty()) {
    if (DEBUG) {
      std::cout << "  No dead induction variables found" << std::endl;
    }
    return false;
  }

  if (DEBUG) {
    std::cout << "  Found " << deadIVs.size() << " potentially dead induction variables" << std::endl;
  }

  // 阶段2：分析安全性
  analyzeSafetyForElimination();

  // 阶段3：执行消除
  bool modified = performInductionVariableElimination();

  if (DEBUG) {
    printDebugInfo();
  }

  modified |= SysYIROptUtils::eliminateRedundantPhisInFunction(F);
  return modified;
}

void InductionVariableEliminationContext::identifyDeadInductionVariables(Function* F) {
  if (DEBUG) {
    std::cout << "  === Phase 1: Identifying Dead Induction Variables ===" << std::endl;
  }

  // 遍历所有循环
  for (const auto& loop_ptr : loopAnalysis->getAllLoops()) {
    Loop* loop = loop_ptr.get();
    
    if (DEBUG) {
      std::cout << "    Analyzing loop: " << loop->getName() << std::endl;
    }

    // 获取循环特征
    const LoopCharacteristics* characteristics = loopCharacteristics->getCharacteristics(loop);
    if (!characteristics) {
      if (DEBUG) {
        std::cout << "      No characteristics available for loop" << std::endl;
      }
      continue;
    }

    if (characteristics->InductionVars.empty()) {
      if (DEBUG) {
        std::cout << "      No induction variables found in loop" << std::endl;
      }
      continue;
    }

    // 检查每个归纳变量是否为死归纳变量
    for (const auto& iv : characteristics->InductionVars) {
      auto deadIV = isDeadInductionVariable(iv.get(), loop);
      if (deadIV) {
        if (DEBUG) {
          std::cout << "      Found potential dead IV: %" << deadIV->phiInst->getName() << std::endl;
        }
        
        // 添加到候选项列表
        loopToDeadIVs[loop].push_back(deadIV.get());
        deadIVs.push_back(std::move(deadIV));
      }
    }
  }

  if (DEBUG) {
    std::cout << "  === End Phase 1: Found " << deadIVs.size() << " candidates ===" << std::endl;
  }
}

std::unique_ptr<DeadInductionVariable> 
InductionVariableEliminationContext::isDeadInductionVariable(const InductionVarInfo* iv, Loop* loop) {
  // 获取 phi 指令
  auto* phiInst = dynamic_cast<PhiInst*>(iv->div);
  if (!phiInst) {
    return nullptr; // 不是 phi 指令
  }

  // 新的逻辑：递归分析整个use-def链，判断是否有真实的使用
  if (!isPhiInstructionDeadRecursively(phiInst, loop)) {
    return nullptr; // 有真实的使用，不能删除
  }

  // 创建死归纳变量信息
  auto deadIV = std::make_unique<DeadInductionVariable>(phiInst, loop);
  deadIV->relatedInsts = collectRelatedInstructions(phiInst, loop);
  
  return deadIV;
}

// 递归分析phi指令及其使用链是否都是死代码
bool InductionVariableEliminationContext::isPhiInstructionDeadRecursively(PhiInst* phiInst, Loop* loop) {
  if (DEBUG) {
    std::cout << "      递归分析归纳变量 " << phiInst->getName() << " 的完整使用链" << std::endl;
  }

  // 使用访问集合避免无限递归
  std::set<Instruction*> visitedInstructions;
  std::set<Instruction*> currentPath; // 用于检测循环依赖
  
  // 核心逻辑：递归分析使用链，寻找任何"逃逸点"
  return isInstructionUseChainDeadRecursively(phiInst, loop, visitedInstructions, currentPath);
}

// 递归分析指令的使用链是否都是死代码
bool InductionVariableEliminationContext::isInstructionUseChainDeadRecursively(
    Instruction* inst, Loop* loop, 
    std::set<Instruction*>& visited, 
    std::set<Instruction*>& currentPath) {
  
  if (DEBUG && visited.size() < 10) { // 限制debug输出
    std::cout << "        分析指令 " << inst->getName() << " (" << inst->getKindString() << ")" << std::endl;
  }
  
  // 避免无限递归
  if (currentPath.count(inst) > 0) {
    // 发现循环依赖，这在归纳变量中是正常的，继续分析其他路径
    if (DEBUG && visited.size() < 10) {
      std::cout << "          发现循环依赖，继续分析其他路径" << std::endl;
    }
    return true; // 循环依赖本身不是逃逸点
  }
  
  if (visited.count(inst) > 0) {
    // 已经分析过这个指令
    return true; // 假设之前的分析是正确的
  }
  
  visited.insert(inst);
  currentPath.insert(inst);
  
  // 1. 检查是否有副作用（逃逸点）
  if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(inst)) {
    if (DEBUG && visited.size() < 10) {
      std::cout << "          指令有副作用，是逃逸点" << std::endl;
    }
    currentPath.erase(inst);
    return false; // 有副作用的指令是逃逸点
  }
  
  // 1.5. 特殊检查：控制流指令永远不是死代码
  auto instKind = inst->getKind();
  if (instKind == Instruction::Kind::kCondBr || 
      instKind == Instruction::Kind::kBr ||
      instKind == Instruction::Kind::kReturn) {
    if (DEBUG && visited.size() < 10) {
      std::cout << "          控制流指令，是逃逸点" << std::endl;
    }
    currentPath.erase(inst);
    return false; // 控制流指令是逃逸点
  }
  
  // 2. 检查指令的所有使用
  bool allUsesAreDead = true;
  for (auto use : inst->getUses()) {
    auto user = use->getUser();
    auto* userInst = dynamic_cast<Instruction*>(user);
    
    if (!userInst) {
      // 被非指令使用（如函数返回值），是逃逸点
      if (DEBUG && visited.size() < 10) {
        std::cout << "          被非指令使用，是逃逸点" << std::endl;
      }
      allUsesAreDead = false;
      break;
    }
    
    // 检查使用是否在循环外（逃逸点）
    if (!loop->contains(userInst->getParent())) {
      if (DEBUG && visited.size() < 10) {
        std::cout << "          在循环外被 " << userInst->getName() << " 使用，是逃逸点" << std::endl;
      }
      allUsesAreDead = false;
      break;
    }
    
    // 特殊检查：如果使用者是循环的退出条件，需要进一步分析
    // 对于用于退出条件的归纳变量，需要更谨慎的处理
    if (isUsedInLoopExitCondition(userInst, loop)) {
      // 修复逻辑：用于循环退出条件的归纳变量通常不应该被消除
      // 除非整个循环都可以被证明是完全无用的（这需要更复杂的分析）
      if (DEBUG && visited.size() < 10) {
        std::cout << "          被用于循环退出条件，是逃逸点（避免破坏循环语义）" << std::endl;
      }
      allUsesAreDead = false;
      break;
    }
    
    // 递归分析使用者的使用链
    if (!isInstructionUseChainDeadRecursively(userInst, loop, visited, currentPath)) {
      allUsesAreDead = false;
      break; // 找到逃逸点，不需要继续分析
    }
  }
  
  currentPath.erase(inst);
  
  if (allUsesAreDead && DEBUG && visited.size() < 10) {
    std::cout << "          指令 " << inst->getName() << " 的所有使用都是死代码" << std::endl;
  }
  
  return allUsesAreDead;
}

// 检查循环是否有副作用
bool InductionVariableEliminationContext::loopHasSideEffects(Loop* loop) {
  // 遍历循环中的所有指令，检查是否有副作用
  for (BasicBlock* bb : loop->getBlocks()) {
    for (auto& inst : bb->getInstructions()) {
      Instruction* instPtr = inst.get();
      
      // 使用副作用分析（如果可用）
      if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(instPtr)) {
        if (DEBUG) {
          std::cout << "          循环中发现有副作用的指令: " << instPtr->getName() << std::endl;
        }
        return true;
      }
      
      // 如果没有副作用分析，使用保守的判断
      if (!sideEffectAnalysis) {
        auto kind = instPtr->getKind();
        // 这些指令通常有副作用
        if (kind == Instruction::Kind::kCall ||
            kind == Instruction::Kind::kStore ||
            kind == Instruction::Kind::kReturn) {
          if (DEBUG) {
            std::cout << "          循环中发现潜在有副作用的指令: " << instPtr->getName() << std::endl;
          }
          return true;
        }
      }
    }
  }
  
  // 重要修复：检查是否为嵌套循环的外层循环
  // 如果当前循环包含其他循环，那么它有潜在的副作用
  for (const auto& loop_ptr : loopAnalysis->getAllLoops()) {
    Loop* otherLoop = loop_ptr.get();
    if(loopAnalysis->getLowestCommonAncestor(otherLoop, loop) == loop) {
      if (DEBUG) {
        std::cout << "          循环 " << loop->getName() << " 是其他循环的外层循环，视为有副作用" << std::endl;
      }
      return true; // 外层循环被视为有副作用
    }
    // if (otherLoop != loop && loop->contains(otherLoop->getHeader())) {
    //   if (DEBUG) {
    //     std::cout << "          循环 " << loop->getName() << " 包含子循环 " << otherLoop->getName() << "，视为有副作用" << std::endl;
    //   }
    //   return true; // 包含子循环的外层循环被视为有副作用
    // }
  }
  
  if (DEBUG) {
    std::cout << "          循环 " << loop->getName() << " 无副作用" << std::endl;
  }
  return false; // 循环无副作用
}

// 检查指令是否被用于循环退出条件
bool InductionVariableEliminationContext::isUsedInLoopExitCondition(Instruction* inst, Loop* loop) {
  // 检查指令是否被循环的退出条件使用
  for (BasicBlock* exitingBB : loop->getExitingBlocks()) {
    auto terminatorIt = exitingBB->terminator();
    if (terminatorIt != exitingBB->end()) {
      Instruction* terminator = terminatorIt->get();
      if (terminator) {
        // 检查终结指令的操作数
        for (size_t i = 0; i < terminator->getNumOperands(); ++i) {
          if (terminator->getOperand(i) == inst) {
            if (DEBUG) {
              std::cout << "          指令 " << inst->getName() << " 用于循环退出条件" << std::endl;
            }
            return true;
          }
        }
        
        // 对于条件分支，还需要检查条件指令的操作数
        if (terminator->getKind() == Instruction::Kind::kCondBr) {
          auto* condBr = dynamic_cast<CondBrInst*>(terminator);
          if (condBr) {
            Value* condition = condBr->getCondition();
            if (condition == inst) {
              if (DEBUG) {
                std::cout << "          指令 " << inst->getName() << " 是循环条件" << std::endl;
              }
              return true;
            }
            
            // 递归检查条件指令的操作数（比如比较指令）
            auto* condInst = dynamic_cast<Instruction*>(condition);
            if (condInst) {
              for (size_t i = 0; i < condInst->getNumOperands(); ++i) {
                if (condInst->getOperand(i) == inst) {
                  if (DEBUG) {
                    std::cout << "          指令 " << inst->getName() << " 用于循环条件的操作数" << std::endl;
                  }
                  return true;
                }
              }
            }
          }
        }
      }
    }
  }
  
  return false;
}


// 检查指令的结果是否未被有效使用
bool InductionVariableEliminationContext::isInstructionResultUnused(Instruction* inst, Loop* loop) {
  // 检查指令的所有使用
  if (inst->getUses().empty()) {
    return true; // 没有使用，肯定是未使用
  }
  
  for (auto use : inst->getUses()) {
    auto user = use->getUser();
    auto* userInst = dynamic_cast<Instruction*>(user);
    
    if (!userInst) {
      return false; // 被非指令使用，认为是有效使用
    }
    
    // 如果在循环外被使用，认为是有效使用
    if (!loop->contains(userInst->getParent())) {
      return false;
    }
    
    // 递归检查使用这个结果的指令是否也是死代码
    // 为了避免无限递归，限制递归深度
    if (!isInstructionEffectivelyDead(userInst, loop, 3)) {
      return false; // 存在有效使用
    }
  }
  
  return true; // 所有使用都是无效的
}

// 检查store指令是否存储到死地址（利用别名分析）
bool InductionVariableEliminationContext::isStoreToDeadLocation(StoreInst* store, Loop* loop) {
  if (!aliasAnalysis) {
    return false; // 没有别名分析，保守返回false
  }
  
  Value* storePtr = store->getPointer();
  
  // 检查是否存储到局部临时变量且该变量在循环外不被读取
  const MemoryLocation* memLoc = aliasAnalysis->getMemoryLocation(storePtr);
  if (!memLoc) {
    return false; // 无法确定内存位置
  }
  
  // 如果是局部数组且只在循环内被访问
  if (memLoc->isLocalArray) {
    // 检查该内存位置是否在循环外被读取
    for (auto* accessInst : memLoc->accessInsts) {
      if (accessInst->getKind() == Instruction::Kind::kLoad) {
        if (!loop->contains(accessInst->getParent())) {
          return false; // 在循环外被读取，不是死存储
        }
      }
    }
    
    if (DEBUG) {
      std::cout << "            存储到局部数组且仅在循环内访问" << std::endl;
    }
    return true; // 存储到仅循环内访问的局部数组
  }
  
  return false;
}

// 检查指令是否有效死代码（带递归深度限制）
bool InductionVariableEliminationContext::isInstructionEffectivelyDead(Instruction* inst, Loop* loop, int maxDepth) {
  if (maxDepth <= 0) {
    return false; // 达到递归深度限制，保守返回false
  }
  
  // 利用副作用分析
  if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(inst)) {
    return false; // 有副作用的指令不是死代码
  }
  
  // 检查特殊指令类型
  switch (inst->getKind()) {
    case Instruction::Kind::kStore:
      // Store指令可能是死存储
      return isStoreToDeadLocation(dynamic_cast<StoreInst*>(inst), loop);
    
    case Instruction::Kind::kCall:
      // 函数调用通常有副作用
      if (sideEffectAnalysis) {
        return !sideEffectAnalysis->hasSideEffect(inst);
      }
      return false; // 保守地认为函数调用有效果
    
    case Instruction::Kind::kReturn:
    case Instruction::Kind::kBr:
    case Instruction::Kind::kCondBr:
      // 控制流指令不是死代码
      return false;
    
    default:
      // 其他指令检查其使用是否有效
      break;
  }
  
  // 检查指令的使用
  if (inst->getUses().empty()) {
    return true; // 没有使用的纯指令是死代码
  }
  
  // 递归检查所有使用
  for (auto use : inst->getUses()) {
    auto user = use->getUser();
    auto* userInst = dynamic_cast<Instruction*>(user);
    
    if (!userInst) {
      return false; // 被非指令使用
    }
    
    if (!loop->contains(userInst->getParent())) {
      return false; // 在循环外被使用
    }
    
    // 递归检查使用者
    if (!isInstructionEffectivelyDead(userInst, loop, maxDepth - 1)) {
      return false; // 存在有效使用
    }
  }
  
  return true; // 所有使用都是死代码
}

// 原有的函数保持兼容，但现在使用增强的死代码分析
bool InductionVariableEliminationContext::isInstructionDeadOrInternalOnly(Instruction* inst, Loop* loop) {
  return isInstructionEffectivelyDead(inst, loop, 5);
}

// 检查store指令是否有后续的load操作
bool InductionVariableEliminationContext::hasSubsequentLoad(StoreInst* store, Loop* loop) {
  if (!aliasAnalysis) {
    // 没有别名分析，保守地假设有后续读取
    return true;
  }
  
  Value* storePtr = store->getPointer();
  const MemoryLocation* storeLoc = aliasAnalysis->getMemoryLocation(storePtr);
  
  if (!storeLoc) {
    // 无法确定内存位置，保守处理
    return true;
  }
  
  // 在循环中和循环后查找对同一位置的load操作
  std::vector<BasicBlock*> blocksToCheck;
  
  // 添加循环内的所有基本块
  for (auto* bb : loop->getBlocks()) {
    blocksToCheck.push_back(bb);
  }
  
  // 添加循环的退出块
  auto exitBlocks = loop->getExitBlocks();
  for (auto* exitBB : exitBlocks) {
    blocksToCheck.push_back(exitBB);
  }
  
  // 搜索load操作
  for (auto* bb : blocksToCheck) {
    for (auto& inst : bb->getInstructions()) {
      if (inst->getKind() == Instruction::Kind::kLoad) {
        LoadInst* loadInst = static_cast<LoadInst*>(inst.get());
        Value* loadPtr = loadInst->getPointer();
        const MemoryLocation* loadLoc = aliasAnalysis->getMemoryLocation(loadPtr);
        
        if (loadLoc && aliasAnalysis->queryAlias(storePtr, loadPtr) != AliasType::NO_ALIAS) {
          // 找到可能读取同一位置的load操作
          if (DEBUG) {
            std::cout << "            找到后续load操作: " << loadInst->getName() << std::endl;
          }
          return true;
        }
      }
    }
  }
  
  // 检查是否通过函数调用间接访问
  for (auto* bb : blocksToCheck) {
    for (auto& inst : bb->getInstructions()) {
      if (inst->getKind() == Instruction::Kind::kCall) {
        CallInst* callInst = static_cast<CallInst*>(inst.get());
        if (callInst && sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(callInst)) {
          // 函数调用可能间接读取内存
          if (DEBUG) {
            std::cout << "            函数调用可能读取内存: " << callInst->getName() << std::endl;
          }
          return true;
        }
      }
    }
  }
  
  if (DEBUG) {
    std::cout << "            未找到后续load操作" << std::endl;
  }
  return false; // 没有找到后续读取
}

// 检查指令是否在循环外有使用
bool InductionVariableEliminationContext::hasUsageOutsideLoop(Instruction* inst, Loop* loop) {
  for (auto use : inst->getUses()) {
    auto user = use->getUser();
    auto* userInst = dynamic_cast<Instruction*>(user);
    
    if (!userInst) {
      // 被非指令使用，可能在循环外
      return true;
    }
    
    if (!loop->contains(userInst->getParent())) {
      // 在循环外被使用
      if (DEBUG) {
        std::cout << "            指令 " << inst->getName() << " 在循环外被 " 
                  << userInst->getName() << " 使用" << std::endl;
      }
      return true;
    }
  }
  
  return false; // 没有循环外使用
}

// 检查store指令是否在循环外有后续的load操作
bool InductionVariableEliminationContext::hasSubsequentLoadOutsideLoop(StoreInst* store, Loop* loop) {
  if (!aliasAnalysis) {
    // 没有别名分析，保守地假设有后续读取
    return true;
  }
  
  Value* storePtr = store->getPointer();
  
  // 检查循环的退出块及其后继
  auto exitBlocks = loop->getExitBlocks();
  std::set<BasicBlock*> visitedBlocks;
  
  for (auto* exitBB : exitBlocks) {
    if (hasLoadInSubtree(exitBB, storePtr, visitedBlocks)) {
      if (DEBUG) {
        std::cout << "            找到循环外的后续load操作" << std::endl;
      }
      return true;
    }
  }
  
  return false; // 没有找到循环外的后续读取
}

// 递归检查基本块子树中是否有对指定位置的load操作
bool InductionVariableEliminationContext::hasLoadInSubtree(BasicBlock* bb, Value* ptr, std::set<BasicBlock*>& visited) {
  if (visited.count(bb) > 0) {
    return false; // 已经访问过，避免无限循环
  }
  visited.insert(bb);
  
  // 检查当前基本块中的指令
  for (auto& inst : bb->getInstructions()) {
    if (inst->getKind() == Instruction::Kind::kLoad) {
      LoadInst* loadInst = static_cast<LoadInst*>(inst.get());
      if (aliasAnalysis && aliasAnalysis->queryAlias(ptr, loadInst->getPointer()) != AliasType::NO_ALIAS) {
        return true; // 找到了对相同或别名位置的load
      }
    } else if (inst->getKind() == Instruction::Kind::kCall) {
      // 函数调用可能间接读取内存
      CallInst* callInst = static_cast<CallInst*>(inst.get());
      if (sideEffectAnalysis && sideEffectAnalysis->hasSideEffect(callInst)) {
        return true; // 保守地认为函数调用可能读取内存
      }
    }
  }
  
  // 递归检查后继基本块（限制深度以避免过度搜索）
  static int searchDepth = 0;
  if (searchDepth < 10) { // 限制搜索深度
    searchDepth++;
    for (auto* succ : bb->getSuccessors()) {
      if (hasLoadInSubtree(succ, ptr, visited)) {
        searchDepth--;
        return true;
      }
    }
    searchDepth--;
  }
  
  return false;
}

std::vector<Instruction*> InductionVariableEliminationContext::collectRelatedInstructions(
    PhiInst* phiInst, Loop* loop) {
  std::vector<Instruction*> relatedInsts;
  
  // 收集所有与该归纳变量相关的指令
  for (auto use : phiInst->getUses()) {
    auto user = use->getUser();
    auto* userInst = dynamic_cast<Instruction*>(user);
    
    if (userInst && loop->contains(userInst->getParent())) {
      relatedInsts.push_back(userInst);
    }
  }
  
  return relatedInsts;
}

void InductionVariableEliminationContext::analyzeSafetyForElimination() {
  if (DEBUG) {
    std::cout << "  === Phase 2: Analyzing Safety for Elimination ===" << std::endl;
  }

  // 为每个死归纳变量检查消除的安全性
  for (auto& deadIV : deadIVs) {
    bool isSafe = isSafeToEliminate(deadIV.get());
    deadIV->canEliminate = isSafe;
    
    if (DEBUG) {
      std::cout << "    Dead IV " << deadIV->phiInst->getName() 
                << ": " << (isSafe ? "SAFE" : "UNSAFE") << " to eliminate" << std::endl;
    }
  }

  if (DEBUG) {
    size_t safeCount = 0;
    for (const auto& deadIV : deadIVs) {
      if (deadIV->canEliminate) safeCount++;
    }
    std::cout << "  === End Phase 2: " << safeCount << " of " << deadIVs.size() 
              << " variables are safe to eliminate ===" << std::endl;
  }
}

bool InductionVariableEliminationContext::isSafeToEliminate(const DeadInductionVariable* deadIV) {
  // 1. 确保归纳变量在循环头
  if (deadIV->phiInst->getParent() != deadIV->containingLoop->getHeader()) {
    if (DEBUG) {
      std::cout << "      Unsafe: phi not in loop header" << std::endl;
    }
    return false;
  }
  
  // 2. 确保相关指令都在循环内
  for (auto* inst : deadIV->relatedInsts) {
    if (!deadIV->containingLoop->contains(inst->getParent())) {
      if (DEBUG) {
        std::cout << "      Unsafe: related instruction outside loop" << std::endl;
      }
      return false;
    }
  }
  
  // 3. 确保没有副作用
  for (auto* inst : deadIV->relatedInsts) {
    if (sideEffectAnalysis) {
      // 使用副作用分析进行精确检查
      if (sideEffectAnalysis->hasSideEffect(inst)) {
        if (DEBUG) {
          std::cout << "      Unsafe: related instruction " << inst->getName() 
                    << " has side effects" << std::endl;
        }
        return false;
      }
    } else {
      // 没有副作用分析时使用保守策略：只允许基本算术运算
      auto kind = inst->getKind();
      if (kind != Instruction::Kind::kAdd && 
          kind != Instruction::Kind::kSub &&
          kind != Instruction::Kind::kMul) {
        if (DEBUG) {
          std::cout << "      Unsafe: related instruction may have side effects (conservative)" << std::endl;
        }
        return false;
      }
    }
  }
  
  // 4. 确保不影响循环的退出条件
  for (BasicBlock* exitingBB : deadIV->containingLoop->getExitingBlocks()) {
    auto terminatorIt = exitingBB->terminator();
    if (terminatorIt != exitingBB->end()) {
      Instruction* terminator = terminatorIt->get();
      if (terminator) {
        for (size_t i = 0; i < terminator->getNumOperands(); ++i) {
          if (terminator->getOperand(i) == deadIV->phiInst) {
            if (DEBUG) {
              std::cout << "      Unsafe: phi used in loop exit condition" << std::endl;
            }
            return false;
          }
        }
      }
    }
  }
  
  return true;
}

bool InductionVariableEliminationContext::performInductionVariableElimination() {
  if (DEBUG) {
    std::cout << "  === Phase 3: Performing Induction Variable Elimination ===" << std::endl;
  }

  bool modified = false;

  for (auto& deadIV : deadIVs) {
    if (!deadIV->canEliminate) {
      continue;
    }

    if (DEBUG) {
      std::cout << "    Eliminating dead IV: " << deadIV->phiInst->getName() << std::endl;
    }

    if (eliminateDeadInductionVariable(deadIV.get())) {
      if (DEBUG) {
        std::cout << "      Successfully eliminated: " << deadIV->phiInst->getName() << std::endl;
      }
      modified = true;
    } else {
      if (DEBUG) {
        std::cout << "      Failed to eliminate: " << deadIV->phiInst->getName() << std::endl;
      }
    }
  }

  if (DEBUG) {
    std::cout << "  === End Phase 3: " << (modified ? "Eliminations performed" : "No eliminations") << " ===" << std::endl;
  }

  return modified;
}

bool InductionVariableEliminationContext::eliminateDeadInductionVariable(DeadInductionVariable* deadIV) {
  // 1. 删除所有相关指令
  for (auto* inst : deadIV->relatedInsts) {
    auto* bb = inst->getParent();
    auto it = bb->findInstIterator(inst);
    if (it != bb->end()) {
        SysYIROptUtils::usedelete(it);
    //   bb->getInstructions().erase(it);
      if (DEBUG) {
        std::cout << "        Removed related instruction: " << inst->getName() << std::endl;
      }
    }
  }

  // 2. 删除 phi 指令
  auto* bb = deadIV->phiInst->getParent();
  auto it = bb->findInstIterator(deadIV->phiInst);
  if (it != bb->end()) {
    SysYIROptUtils::usedelete(it);
    // bb->getInstructions().erase(it);
    if (DEBUG) {
      std::cout << "        Removed phi instruction: " << deadIV->phiInst->getName() << std::endl;
    }
    return true;
  }

  return false;
}

void InductionVariableEliminationContext::printDebugInfo() {
  if (!DEBUG) return;

  std::cout << "\n=== Induction Variable Elimination Summary ===" << std::endl;
  std::cout << "Total dead IVs found: " << deadIVs.size() << std::endl;
  
  size_t eliminatedCount = 0;
  for (auto& [loop, loopDeadIVs] : loopToDeadIVs) {
    size_t loopEliminatedCount = 0;
    for (auto* deadIV : loopDeadIVs) {
      if (deadIV->canEliminate) {
        loopEliminatedCount++;
        eliminatedCount++;
      }
    }
    
    if (loopEliminatedCount > 0) {
      std::cout << "Loop " << loop->getName() << ": " << loopEliminatedCount 
                << " of " << loopDeadIVs.size() << " IVs eliminated" << std::endl;
    }
  }
  
  std::cout << "Total eliminated: " << eliminatedCount << " of " << deadIVs.size() << std::endl;
  std::cout << "=============================================" << std::endl;
}

} // namespace sysy
