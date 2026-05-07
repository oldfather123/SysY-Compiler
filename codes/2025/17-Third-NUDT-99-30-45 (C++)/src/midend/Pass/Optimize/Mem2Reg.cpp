#include "Mem2Reg.h" // 包含 Mem2Reg 遍的头文件
#include "Dom.h"     // 包含支配树分析的头文件
#include "Liveness.h"
#include "AliasAnalysis.h" // 包含别名分析
#include "SideEffectAnalysis.h" // 包含副作用分析
#include "IR.h"      // 包含 IR 相关的定义
#include "SysYIROptUtils.h"
#include <cassert>   // 用于断言
#include <iostream>  // 用于调试输出

namespace sysy {

void *Mem2Reg::ID = (void *)&Mem2Reg::ID;

void Mem2RegContext::run(Function *func, AnalysisManager *AM) {
  if (func->getBasicBlocks().empty()) {
    return;
  }

  // 清空所有状态，确保每次运行都是新的状态
  promotableAllocas.clear();
  allocaToPhiMap.clear();
  allocaToValueStackMap.clear();
  allocaToStoresMap.clear();
  allocaToDefBlocksMap.clear();

  // 获取支配树分析结果
  dt = AM->getAnalysisResult<DominatorTree, DominatorTreeAnalysisPass>(func);
  assert(dt && "DominatorTreeAnalysisResult not available for Mem2Reg!");

  // --------------------------------------------------------------------
  // 阶段1: 识别可提升的 AllocaInst 并收集其 Store 指令
  // --------------------------------------------------------------------
  // 遍历函数入口块?中的所有指令，寻找 AllocaInst
  // 必须是要入口块的吗
  for (auto &inst : func->getEntryBlock()->getInstructions_Range()) {
    Value *allocainst = inst.get();
    if (auto alloca = dynamic_cast<AllocaInst *>(allocainst)) {
      if (isPromotableAlloca(alloca)) {
        promotableAllocas.push_back(alloca);
        collectStores(alloca); // 收集所有对该 alloca 的 store
      }
    }
  }

  // --------------------------------------------------------------------
  // 阶段2: 插入 Phi 指令
  // --------------------------------------------------------------------
  for (auto alloca : promotableAllocas) {
    // 为每个可提升的 alloca 插入 Phi 指令
    insertPhis(alloca, allocaToDefBlocksMap[alloca]);
  }

  // --------------------------------------------------------------------
  // 阶段3: 变量重命名
  // --------------------------------------------------------------------
  // 为每个可提升的 alloca 初始化其值栈
  for (auto alloca : promotableAllocas) {
    // 初始值通常是 undef 或 null，取决于 IR 类型系统
    UndefinedValue *undefValue =  UndefinedValue::get(alloca->getType()->as<PointerType>()->getBaseType());
    allocaToValueStackMap[alloca].push(undefValue); // 压入一个初始的“未定义”值
  }

  // 从入口基本块开始，对支配树进行 DFS 遍历，进行变量重命名
  renameVariables(func->getEntryBlock()); // 第一个参数 alloca 在这里不使用，因为是递归入口点

  // --------------------------------------------------------------------
  // 阶段4: 清理
  // --------------------------------------------------------------------
  cleanup();
}

// 判断一个 AllocaInst 是否可以被提升到寄存器
bool Mem2RegContext::isPromotableAlloca(AllocaInst *alloca) {
  // 1. 必须是标量类型（非数组、非结构体）sysy不支持结构体
  if (alloca->getType()->as<PointerType>()->getBaseType()->isArray()) {
    return false;
  }

  // 2. 其所有用途都必须是 LoadInst 或 StoreInst
  // (或 GetElementPtrInst，但 GEP 的结果也必须只被 Load/Store 使用)
  for (auto use : alloca->getUses()) {
    auto user = use->getUser();
    if (!user)
      return false; // 用户无效

    if (dynamic_cast<LoadInst *>(user)) {
      // OK
    } else if (dynamic_cast<StoreInst *>(user)) {
      // OK
    } else if (auto gep = dynamic_cast<GetElementPtrInst *>(user)) {
      // 如果是 GetElementPtrInst (GEP)
      // 需要判断这个 GEP 是否代表了数组元素的访问，而非简单的指针操作
      // LLVM 的 mem2reg 通常不提升用于数组元素访问的 alloca。
      // 启发式判断：
      // 如果 GEP 有多个索引（例如 `getelementptr i32, i32* %ptr, i32 0, i32 %idx`），
      // 或者第一个索引（对于指针类型）不是常量 0，则很可能是数组访问。
      // 对于 `alloca i32* %a.param` (对应 `int a[]` 参数)，其 `allocatedType()` 是 `i32*`。
      // 访问 `a[i]` 会生成类似 `getelementptr i32, i32* %a.param, i32 %i` 的 GEP。
      // 这种 GEP 有两个操作数：基指针和索引。

      // 检查 GEP 的操作数数量和索引值
      // GEP 的操作数通常是：<base_pointer>, <index_1>, <index_2>, ...
      // 对于一个 `i32*` 类型的 `alloca`，如果它被 GEP 使用，那么 GEP 的第一个索引通常是 `0`
      // （表示解引用指针本身），后续索引才是数组元素的索引。
      // 如果 GEP 的操作数数量大于 2 (即 `base_ptr` 和 `index_0` 之外还有其他索引)，
      // 或者 `index_0` 不是常量 0，则它可能是一个复杂的数组访问。
      // 假设 `gep->getNumOperands()` 和 `gep->getOperand(idx)->getValue()`
      // 假设 `ConstantInt` 类用于表示常量整数值
      if (gep->getNumOperands() > 2) { // 如果有超过一个索引（除了基指针的第一个隐式索引）
        // std::cerr << "Mem2Reg: Not promotable (GEP with multiple indices): " << alloca->name() << std::endl;
        return false; // 复杂 GEP，通常表示数组或结构体字段访问
      }
      if (gep->getNumOperands() == 2) {                            // 只有基指针和一个索引
        Value *firstIndexVal = gep->getOperand(1); // 获取第一个索引值
        if (auto constInt = dynamic_cast<ConstantInteger *>(firstIndexVal)) {
          if (constInt->getInt() != 0) {
            // std::cerr << "Mem2Reg: Not promotable (GEP with non-zero first index): " << alloca->name() << std::endl;
            return false; // 索引不是0，表示访问数组的非第一个元素
          }
        } else {
          // std::cerr << "Mem2Reg: Not promotable (GEP with non-constant first index): " << alloca->name() <<
          // std::endl;
          return false; // 索引不是常量，表示动态数组访问
        }
      }

      // 此外，GEP 的结果也必须只被 LoadInst 或 StoreInst 使用
      for (auto gep_use : gep->getUses()) {
        auto gep_user = gep_use->getUser();
        if (!gep_user) {
          // std::cerr << "Mem2Reg: Not promotable (GEP result null user): " << alloca->name() << std::endl;
          return false;
        }
        if (!dynamic_cast<LoadInst *>(gep_user) && !dynamic_cast<StoreInst *>(gep_user)) {
          // std::cerr << "Mem2Reg: Not promotable (GEP result used by non-load/store): " << alloca->name() <<
          // std::endl;
          return false; // GEP 结果被其他指令使用，地址逃逸或复杂用途
        }
      }
    } else {
      // 其他类型的用户，如 CallInst (如果地址逃逸)，则不能提升
      return false;
    }
  }
  // 3. 不能是 volatile 内存访问 (假设 AllocaInst 有 isVolatile() 方法)
  // if (alloca->isVolatile()) return false; // 如果有这样的属性

  return true;
}

// 收集所有对给定 AllocaInst 进行存储的 StoreInst
void Mem2RegContext::collectStores(AllocaInst *alloca) {
  // 遍历 alloca 的所有用途
  for (auto use : alloca->getUses()) {
    auto user = use->getUser();
    if (!user)
      continue;

    if (auto storeInst = dynamic_cast<StoreInst *>(user)) {
      allocaToStoresMap[alloca].insert(storeInst);
      allocaToDefBlocksMap[alloca].insert(storeInst->getParent());
    } else if (auto gep = dynamic_cast<GetElementPtrInst *>(user)) {
      // 如果是 GEP，递归收集其下游的 store
      for (auto gep_use : gep->getUses()) {
        if (auto gep_store = dynamic_cast<StoreInst *>(gep_use->getUser())) {
          allocaToStoresMap[alloca].insert(gep_store);
          allocaToDefBlocksMap[alloca].insert(gep_store->getParent());
        }
      }
    }
  }
}

// 为给定的 AllocaInst 插入必要的 Phi 指令
void Mem2RegContext::insertPhis(AllocaInst *alloca, const std::unordered_set<BasicBlock *> &defBlocks) {
  std::queue<BasicBlock *> workQueue;
  std::unordered_set<BasicBlock *> phiHasBeenInserted; // 记录已插入 Phi 的基本块

  // 将所有定义块加入工作队列
  for (auto bb : defBlocks) {
    workQueue.push(bb);
  }

  while (!workQueue.empty()) {
    BasicBlock *currentDefBlock = workQueue.front();
    workQueue.pop();

    // 遍历当前定义块的支配边界 (Dominance Frontier)
    const std::set<BasicBlock *> *frontierBlocks = dt->getDominanceFrontier(currentDefBlock);
    for (auto frontierBlock : *frontierBlocks) {
      // 如果该支配边界块还没有为当前 alloca 插入 Phi 指令
      if (phiHasBeenInserted.find(frontierBlock) == phiHasBeenInserted.end()) {
        // 在支配边界块的开头插入一个新的 Phi 指令
        // Phi 指令的类型与 alloca 的类型指向的类型相同

        builder->setPosition(frontierBlock, frontierBlock->begin()); // 设置插入位置为基本块开头
        PhiInst *phiInst = builder->createPhiInst(alloca->getAllocatedType(), {}, {}, "");
        
        allocaToPhiMap[alloca][frontierBlock] = phiInst; // 记录 Phi 指令

        phiHasBeenInserted.insert(frontierBlock); // 标记已插入 Phi

        // 如果这个支配边界块本身也是一个定义块（即使没有 store，但插入了 Phi），
        // 那么它的支配边界也可能需要插入 Phi
        // 例如一个xx型的cfg，如果在第一个交叉处插入phi节点，那么第二个交叉处可能也需要插入phi
        workQueue.push(frontierBlock);
      }
    }
  }
}

// 对支配树进行深度优先遍历，重命名变量并替换 load/store 指令
// 移除了 AllocaInst *currentAlloca 参数，因为这个函数是为整个基本块处理所有可提升的 Alloca
void Mem2RegContext::renameVariables(BasicBlock *currentBB) {
  // 1. 在函数开始时，记录每个 promotableAlloca 的当前栈深度。
  // 这将用于在函数返回时精确地回溯栈状态。
  std::map<AllocaInst *, size_t> originalStackSizes;
  for (auto alloca : promotableAllocas) {
    originalStackSizes[alloca] = allocaToValueStackMap[alloca].size();
  }

  // --------------------------------------------------------------------
  // 处理当前基本块的指令
  // --------------------------------------------------------------------
  for (auto instIter = currentBB->getInstructions().begin(); instIter != currentBB->getInstructions().end();) {
    Instruction *inst = instIter->get();
      bool instDeleted = false;

    // 处理 Phi 指令 (如果是当前 alloca 的 Phi)
    if (auto phiInst = dynamic_cast<PhiInst *>(inst)) {
      // 检查这个 Phi 是否是为某个可提升的 alloca 插入的
      for (auto alloca : promotableAllocas) {
        if (allocaToPhiMap[alloca].count(currentBB) && allocaToPhiMap[alloca][currentBB] == phiInst) {
          // 为 Phi 指令的输出创建一个新的 SSA 值，并压入值栈
          allocaToValueStackMap[alloca].push(phiInst);
          if (DEBUG) {
            std::cout << "Mem2Reg: Pushed Phi " << (phiInst->getName().empty() ? "anonymous" : phiInst->getName()) << " for alloca " << alloca->getName()
              << ". Stack size: " << allocaToValueStackMap[alloca].size() << std::endl;
          }
          break; // 找到对应的 alloca，处理下一个指令
        }
      }
    }
    // 处理 LoadInst
    else if (auto loadInst = dynamic_cast<LoadInst *>(inst)) {
      for (auto alloca : promotableAllocas) {
        // 检查 LoadInst 的指针是否直接是 alloca，或者是指向 alloca 的 GEP
        Value *ptrOperand = loadInst->getPointer();
        if (ptrOperand == alloca || (dynamic_cast<GetElementPtrInst *>(ptrOperand) &&
                                     dynamic_cast<GetElementPtrInst *>(ptrOperand)->getBasePointer() == alloca)) {
          assert(!allocaToValueStackMap[alloca].empty() && "Value stack empty for alloca during load replacement!");
          if (DEBUG) {
            std::cout << "Mem2Reg: Replacing load "
                      << (ptrOperand->getName().empty() ? "anonymous" : ptrOperand->getName()) << " with SSA value "
                      << (allocaToValueStackMap[alloca].top()->getName().empty()
                              ? "anonymous"
                              : allocaToValueStackMap[alloca].top()->getName())
                      << " for alloca " << alloca->getName() << std::endl;
            std::cout << "Mem2Reg: allocaToValueStackMap[" << alloca->getName()
                      << "] size: " << allocaToValueStackMap[alloca].size() << std::endl;
          }
          loadInst->replaceAllUsesWith(allocaToValueStackMap[alloca].top());
          instIter = SysYIROptUtils::usedelete(instIter);
          instDeleted = true;
          break;
        }
      }
    }
    // 处理 StoreInst
    else if (auto storeInst = dynamic_cast<StoreInst *>(inst)) {
      for (auto alloca : promotableAllocas) {
        // 检查 StoreInst 的指针是否直接是 alloca，或者是指向 alloca 的 GEP
        Value *ptrOperand = storeInst->getPointer();
        if (ptrOperand == alloca || (dynamic_cast<GetElementPtrInst *>(ptrOperand) &&
                                     dynamic_cast<GetElementPtrInst *>(ptrOperand)->getBasePointer() == alloca)) {
          if (DEBUG) {
            std::cout << "Mem2Reg: Replacing store to "
                      << (ptrOperand->getName().empty() ? "anonymous" : ptrOperand->getName()) << " with SSA value "
                      << (storeInst->getValue()->getName().empty() ? "anonymous" : storeInst->getValue()->getName())
                      << " for alloca " << alloca->getName() << std::endl;
            std::cout << "Mem2Reg: allocaToValueStackMap[" << alloca->getName()
                      << "] size before push: " << allocaToValueStackMap[alloca].size() << std::endl;
          }
          allocaToValueStackMap[alloca].push(storeInst->getValue());
          instIter = SysYIROptUtils::usedelete(instIter);
          instDeleted = true;
          if (DEBUG) {
            std::cout << "Mem2Reg: allocaToValueStackMap[" << alloca->getName()
                      << "] size after push: " << allocaToValueStackMap[alloca].size() << std::endl;
          }
          break;
        }
      }
    }
    if (!instDeleted) {
      ++instIter; // 如果指令没有被删除，移动到下一个
    }
  }
  // --------------------------------------------------------------------
  // 处理后继基本块的 Phi 指令参数
  // --------------------------------------------------------------------
  for (auto successorBB : currentBB->getSuccessors()) {
    if (!successorBB)
      continue;
    for (auto alloca : promotableAllocas) {
      // 如果后继基本块包含为当前 alloca 插入的 Phi 指令
      if (allocaToPhiMap[alloca].count(successorBB)) {
        auto phiInst = allocaToPhiMap[alloca][successorBB];
        // 为 Phi 指令添加来自当前基本块的参数
        // 参数值是当前 alloca 值栈顶部的 SSA 值
        assert(!allocaToValueStackMap[alloca].empty() && "Value stack empty for alloca when setting phi operand!");
        phiInst->addIncoming(allocaToValueStackMap[alloca].top(), currentBB);
        if (DEBUG) {
          std::cout << "Mem2Reg: Added incoming arg to Phi "
                    << (phiInst->getName().empty() ? "anonymous" : phiInst->getName()) << " from "
                    << currentBB->getName() << " with value "
                    << (allocaToValueStackMap[alloca].top()->getName().empty()
                            ? "anonymous"
                            : allocaToValueStackMap[alloca].top()->getName())
                    << std::endl;
        }
      }
    }
  }
  // --------------------------------------------------------------------
  // 递归访问支配树的子节点
  // --------------------------------------------------------------------
  const std::set<BasicBlock *> *dominatedBlocks = dt->getDominatorTreeChildren(currentBB);
  if (dominatedBlocks) { // 检查是否存在子节点
    if(DEBUG){
      std::cout << "Mem2Reg: Processing dominated blocks for " << currentBB->getName() << std::endl;
      for (auto dominatedBB : *dominatedBlocks) {
        std::cout << "Mem2Reg: Dominated block: " << (dominatedBB ? dominatedBB->getName() : "null") << std::endl;
      }
    }
    for (auto dominatedBB : *dominatedBlocks) {
      if (dominatedBB) { // 确保子块有效
        if (DEBUG) {
          std::cout << "Mem2Reg: Recursively renaming variables in dominated block: " << dominatedBB->getName()
                    << std::endl;
        }
        renameVariables(dominatedBB); // 递归调用，不再传递 currentAlloca
      }
    }
  }

  // --------------------------------------------------------------------
  // 退出基本块时，弹出在此块中压入值栈的 SSA 值，恢复栈到进入该块时的状态
  // --------------------------------------------------------------------
  for (auto alloca : promotableAllocas) {
    while (allocaToValueStackMap[alloca].size() > originalStackSizes[alloca]) {
      if (DEBUG) {
        std::cout << "Mem2Reg: Popping value "
                  << (allocaToValueStackMap[alloca].top()->getName().empty()
                          ? "anonymous"
                          : allocaToValueStackMap[alloca].top()->getName())
                  << " for alloca " << alloca->getName() << ". Stack size: " << allocaToValueStackMap[alloca].size()
                  << " -> " << (allocaToValueStackMap[alloca].size() - 1) << std::endl;
      }
      allocaToValueStackMap[alloca].pop();
    }
  }

}

// 删除所有原始的 AllocaInst、LoadInst 和 StoreInst
void Mem2RegContext::cleanup() {
  for (auto alloca : promotableAllocas) {
    if (alloca && alloca->getParent()) {
      // 删除 alloca 指令本身
      SysYIROptUtils::usedelete(alloca);
      
      // std::cerr << "Mem2Reg: Deleted alloca " << alloca->name() << std::endl;
    }
  }
  // LoadInst 和 StoreInst 已经在 renameVariables 阶段被删除了
}

// Mem2Reg 遍的 runOnFunction 方法实现
bool Mem2Reg::runOnFunction(Function *F, AnalysisManager &AM) {
  // 记录初始的指令数量，用于判断优化是否发生了改变
  size_t initial_inst_count = 0;
  for (auto &bb : F->getBasicBlocks()) {
    initial_inst_count += bb->getInstructions().size();
  }

  Mem2RegContext ctx(builder);
  ctx.run(F, &AM); // 运行 Mem2Reg 优化

  // 运行优化后，再次计算指令数量
  size_t final_inst_count = 0;
  for (auto &bb : F->getBasicBlocks()) {
    final_inst_count += bb->getInstructions().size();
  }

  // 如果指令数量发生变化（通常是减少，因为 load/store 被删除，phi 被添加），说明 IR 被修改了
  // TODO：不保险，后续修改为更精确的判断
  // 直接在添加和删除指令时维护changed值
  bool changed = (initial_inst_count != final_inst_count);

  // 如果 IR 被修改，则使相关的分析结果失效
  if (changed) {
    // Mem2Reg 会显著改变 IR 结构，特别是数据流和控制流（通过 Phi）。
    // 这会使几乎所有数据流分析和部分控制流分析失效。
    // AM.invalidateAnalysis(&DominatorTreeAnalysisPass::ID, F); // 支配树可能间接改变（如果基本块被删除）
    // AM.invalidateAnalysis(&LivenessAnalysisPass::ID, F); // 活跃性分析肯定失效
    // AM.invalidateAnalysis(&LoopInfoAnalysisPass::ID, F); // 循环信息可能失效
    // AM.invalidateAnalysis(&SideEffectInfoAnalysisPass::ID); // 副作用分析可能失效（如果 Alloca/Load/Store
    // 被替换为寄存器）
    // ... 其他数据流分析，如到达定义、可用表达式等，也应失效
  }
  return changed;
}

// 声明Mem2Reg遍的分析依赖和失效信息
void Mem2Reg::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // Mem2Reg 强烈依赖于支配树分析来插入 Phi 指令
  analysisDependencies.insert(&DominatorTreeAnalysisPass::ID); // 假设 DominatorTreeAnalysisPass 的 ID

  // Mem2Reg 会删除 Alloca/Load/Store 指令，插入 Phi 指令，这会大幅改变 IR 结构。
  // 因此，它会使许多分析结果失效。
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID); // 支配树可能受影响
  analysisInvalidations.insert(&LivenessAnalysisPass::ID); // 活跃性分析肯定失效
  analysisInvalidations.insert(&SysYAliasAnalysisPass::ID); // 别名分析必须失效，因为Mem2Reg改变了内存访问模式
  analysisInvalidations.insert(&SysYSideEffectAnalysisPass::ID); // 副作用分析也可能失效
  // analysisInvalidations.insert(&LoopInfoAnalysisPass::ID); // 循环信息可能失效
  // 其他所有依赖于数据流或 IR 结构的分析都可能失效。
}

} // namespace sysy