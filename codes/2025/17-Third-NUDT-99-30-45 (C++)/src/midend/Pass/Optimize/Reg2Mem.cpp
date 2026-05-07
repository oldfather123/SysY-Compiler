#include "Reg2Mem.h"
#include "SysYIROptUtils.h"
#include "SysYIRPrinter.h"

extern int DEBUG; // 全局调试标志

namespace sysy {

void *Reg2Mem::ID = (void *)&Reg2Mem::ID;

void Reg2MemContext::run(Function *func) {
  if (func->getBasicBlocks().empty()) {
    return;
  }

  // 清空状态，确保每次运行都是新的
  valueToAllocaMap.clear();

  // 阶段1: 识别并为 SSA Value 分配 AllocaInst
  allocateMemoryForSSAValues(func);

  // 阶段2: 将 Phi 指令转换为 Load/Store 逻辑 (此阶段需要先于通用 Load/Store 插入)
  // 这样做是因为 Phi 指令的特殊性，它需要在前驱块的末尾插入 Store
  // 如果先处理通用 Load/Store，可能无法正确处理 Phi 的复杂性
  rewritePhis(func); // Phi 指令可能在 rewritePhis 中被删除或标记删除

  // 阶段3: 将其他 SSA Value 的使用替换为 Load/Store
  insertLoadsAndStores(func);

  // 阶段4: 清理（删除不再需要的 Phi 指令）
  cleanup(func);
}

bool Reg2MemContext::isPromotableToMemory(Value *val) {
  // 参数和指令结果是 SSA 值
  if(DEBUG){
    // if(val->getName() == ""){
    //   assert(false && "Value name should not be empty in Reg2MemContext::isPromotableToMemory");
    // }
    // std::cout << "Checking if value is promotable to memory: " << val->getName() << std::endl;
  }
  // if (dynamic_cast<Argument *>(val) || dynamic_cast<Instruction *>(val)) {
  //   // 如果值已经是指针类型，则通常不为其分配额外的内存，因为它已经是一个地址。
  //   // （除非我们想将其值也存储起来，这通常不用于 Reg2Mem）
  //   // // Reg2Mem 关注的是将非指针值从寄存器语义转换为内存语义。
  //   if (val->getType()->isPointer()) {
  //     return false;
  //   }
  //   return true;
  // }
  // 1. 如果是 Argument，则可以提升到内存
  if (dynamic_cast<Argument *>(val)) {
    // 参数类型（i32, i32* 等）都可以为其分配内存
    // 因为它们在 Mem2Reg 逆操作中，被认为是从寄存器分配到内存
    return true;
  }
  if (dynamic_cast<PhiInst *>(val)) {
    // Phi 指令的结果也是一个 SSA 值，需要将其转换为 Load/Store
    return true;
  }
  return false;
}

void Reg2MemContext::allocateMemoryForSSAValues(Function *func) {
  // AllocaInst 必须在函数的入口基本块中
  BasicBlock *entryBlock = func->getEntryBlock();
  if (!entryBlock) {
    return; // 函数可能没有入口块 (例如声明)
  }

  // 1. 为函数参数分配内存
  builder->setPosition(entryBlock, entryBlock->begin()); // 确保在入口块的开始位置插入
  // for (auto arg : func->getArguments()) {
  //   // 默认情况下，将所有参数是提升到内存
  //   if (isPromotableToMemory(arg)) {
  //     // 参数的类型就是 AllocaInst 需要分配的类型
  //     AllocaInst *alloca = builder->createAllocaInst(Type::getPointerType(arg->getType()), arg->getName() + ".reg2mem");
  //     // 将参数值 store 到 alloca 中 (这是 Mem2Reg 逆转的关键一步)
  //     valueToAllocaMap[arg] = alloca;

  //     // 确保 alloca 位于入口块的顶部，但在所有参数的 store 指令之前
  //     // 通常 alloca 都在 entry block 的最开始
  //     // 这里我们只是创建，并让 builder 决定插入位置 (通常在当前插入点)
  //     // 如果需要严格控制顺序，可能需要手动 insert 到 instruction list
  //   }
  // }

  // 2. 为指令结果分配内存
  // 遍历所有基本块和指令，找出所有需要分配 Alloca 的指令结果
  for (auto &bb : func->getBasicBlocks()) {
    for (auto &inst : bb->getInstructions_Range()) {
      // SysYPrinter::printInst(inst.get());
      // 只有有结果的指令才可能需要分配内存
      // (例如 BinaryInst, CallInst, LoadInst, PhiInst 等)
      // StoreInst, BranchInst, ReturnInst 等没有结果的指令不需要
      
      if (dynamic_cast<AllocaInst*>(inst.get()) || inst.get()->getType()->isVoid()) {
        continue;
      }

      if (isPromotableToMemory(inst.get())) {
        // 为指令的结果分配内存
        // AllocaInst 应该在入口块，而不是当前指令所在块
        // 这里我们只是创建，并稍后调整其位置
        // 通常的做法是在循环结束后统一将 alloca 放到 entryBlock 的顶部
        AllocaInst *alloca = builder->createAllocaInst(Type::getPointerType(inst.get()->getType()), inst.get()->getName() + ".reg2mem");
        valueToAllocaMap[inst.get()] = alloca;
      }
    }
  }
  Instruction *firstNonAlloca = nullptr;
  for (auto instIter = entryBlock->getInstructions().begin(); instIter != entryBlock->getInstructions().end(); instIter++) {
    if (!dynamic_cast<AllocaInst*>(instIter->get())) {
      firstNonAlloca = instIter->get();
      break;
    }
  }

  if (firstNonAlloca) {
    builder->setPosition(entryBlock, entryBlock->findInstIterator(firstNonAlloca));
  } else { // 如果 entryBlock 只有 AllocaInst 或为空，则设置到 terminator 前
    builder->setPosition(entryBlock, entryBlock->terminator());
  }

  // 插入所有参数的初始 Store 指令
  // for (auto arg : func->getArguments()) {
  //     if (valueToAllocaMap.count(arg)) { // 检查是否为其分配了 alloca
  //         builder->createStoreInst(arg, valueToAllocaMap[arg]);
  //     }
  // }
  
  builder->setPosition(entryBlock, entryBlock->terminator());
}

void Reg2MemContext::rewritePhis(Function *func) {
  std::vector<PhiInst *> phisToErase; // 收集要删除的 Phi

  // 遍历所有基本块和其中的指令，查找 Phi 指令
  for (auto &bb : func->getBasicBlocks()) {
    // auto insts = bb->getInstructions(); // 复制一份，因为要修改
    for (auto instIter = bb->getInstructions().begin(); instIter != bb->getInstructions().end(); instIter++) {
      Instruction *inst = instIter->get();
      if (auto phiInst = dynamic_cast<PhiInst *>(inst)) {
        // 检查 Phi 指令是否是需要处理的 SSA 值
        if (valueToAllocaMap.count(phiInst)) {
          AllocaInst *alloca = valueToAllocaMap[phiInst];

          // 1. 为 Phi 指令的每个入边，在前驱块的末尾插入 Store 指令
          // PhiInst 假设有 getIncomingValues() 和 getIncomingBlocks()
          for (unsigned i = 0; i < phiInst->getNumIncomingValues(); ++i) {         // 假设 PhiInst 是通过操作数来管理入边的
            Value *incomingValue = phiInst->getIncomingValue(i);                   // 获取入值
            BasicBlock *incomingBlock = phiInst->getIncomingBlock(i); // 获取对应的入块

            // 在入块的跳转指令之前插入 StoreInst
            // 需要找到 incomingBlock 的终结指令 (Terminator Instruction)
            // 并将 StoreInst 插入到它前面
            if (incomingBlock->terminator()->get()->isTerminator()) {
              builder->setPosition(incomingBlock, incomingBlock->terminator());
            } else {
              // 如果没有终结指令，插入到末尾
              builder->setPosition(incomingBlock, incomingBlock->end());
            }
            builder->createStoreInst(incomingValue, alloca);
          }

          // 2. 在当前 Phi 所在基本块的开头，插入 Load 指令
          // 将 Load 指令插入到 Phi 指令之后，因为 Phi 指令即将被删除
          builder->setPosition(bb.get(), bb.get()->findInstIterator(phiInst));
          LoadInst *newLoad = builder->createLoadInst(alloca);

          // 3. 将 Phi 指令的所有用途替换为新的 Load 指令
          phiInst->replaceAllUsesWith(newLoad);

          // 标记 Phi 指令待删除
          phisToErase.push_back(phiInst);
        }
      }
    }
  }

  // 实际删除 Phi 指令
  for (auto phi : phisToErase) {
    if (phi && phi->getParent()) {
      SysYIROptUtils::usedelete(phi);
    }
  }
}

void Reg2MemContext::insertLoadsAndStores(Function *func) {
  // 收集所有需要替换的 uses，避免在迭代时修改 use 链表
  std::vector<std::pair<Use *, LoadInst *>> usesToReplace;
  std::vector<Instruction *> instsToStore; // 收集需要插入 Store 的指令

  // 遍历所有基本块和指令
  for (auto &bb : func->getBasicBlocks()) {
    for (auto instIter = bb->getInstructions().begin(); instIter != bb->getInstructions().end(); instIter++) {
      Instruction *inst = instIter->get();

      // 如果指令有结果且我们为其分配了 alloca (Phi 已在 rewritePhis 处理)
      // 并且其类型不是 void
      if (!inst->getType()->isVoid() && valueToAllocaMap.count(inst)) {
        // 在指令之后插入 Store 指令
        // StoreInst 应该插入到当前指令之后
        builder->setPosition(bb.get(), bb.get()->findInstIterator(inst));
        builder->createStoreInst(inst, valueToAllocaMap[inst]);
      }

      // 处理指令的操作数：如果操作数是一个 SSA 值，且为其分配了 alloca
      // (并且这个操作数不是 Phi Inst 的 incoming value，因为 Phi 的 incoming value 已经在 rewritePhis 中处理了)
      // 注意：Phi Inst 的操作数是特殊的，它们表示来自不同前驱块的值。
      // 这里的处理主要是针对非 Phi 指令的操作数。
      for (auto use = inst->getUses().begin(); use != inst->getUses().end(); ++use) {
        // 如果当前 use 的 Value 是一个 Instruction 或 Argument
        Value *operand = use->get()->getValue();
        if (isPromotableToMemory(operand) && valueToAllocaMap.count(operand)) {
          // 确保这个 operand 不是一个即将被删除的 Phi 指令
          // (在 rewritePhis 阶段，Phi 已经被处理并可能被标记删除)
          // 或者检查 use 的 user 不是 PhiInst
          if (dynamic_cast<PhiInst *>(inst)) {
            continue; // Phi 的操作数已在 rewritePhis 中处理
          }

          AllocaInst *alloca = valueToAllocaMap[operand];

          // 在使用点之前插入 Load 指令
          // LoadInst 应该插入到使用它的指令之前
          builder->setPosition(bb.get(), bb.get()->findInstIterator(inst));
          LoadInst *newLoad = builder->createLoadInst(alloca);

          // 记录要替换的 use
          usesToReplace.push_back({use->get(), newLoad});
        }
      }
    }
  }

  // 执行所有替换操作
  for (auto &pair : usesToReplace) {
    pair.first->setValue(pair.second); // 替换 use 的 Value
  }
}

void Reg2MemContext::cleanup(Function *func) {
  // 此时，所有原始的 Phi 指令应该已经被删除。
  // 如果有其他需要删除的临时指令，可以在这里处理。
  // 通常，Reg2Mem 的清理比 Mem2Reg 简单，因为主要是在插入指令。
  // 这里可以作为一个占位符，以防未来有其他清理需求。
}

bool Reg2Mem::runOnFunction(Function *F, AnalysisManager &AM) {
  // 记录初始指令数量
  size_t initial_inst_count = 0;
  for (auto &bb : F->getBasicBlocks()) {
    initial_inst_count += bb->getInstructions().size();
  }

  Reg2MemContext ctx(builder); // 假设 builder 是一个全局或可访问的 IRBuilder 实例
  ctx.run(F);

  // 记录最终指令数量
  size_t final_inst_count = 0;
  for (auto &bb : F->getBasicBlocks()) {
    final_inst_count += bb->getInstructions().size();
  }
  // TODO: 添加更精确的变化检测逻辑，例如在run函数中维护changed状态
  bool changed = (initial_inst_count != final_inst_count); // 粗略判断是否改变

  if (changed) {
    // Reg2Mem 会显著改变 IR 结构，特别是数据流。
    // 它会插入大量的 Load/Store 指令，改变 Value 的来源。
    // 这会使几乎所有数据流分析失效。
    // 例如：
    // AM.invalidateAnalysis(&DominatorTreeAnalysisPass::ID, F); // 如果基本块结构改变，可能失效
    // AM.invalidateAnalysis(&LivenessAnalysisPass::ID, F);     // 活跃性分析肯定失效
    // AM.invalidateAnalysis(&DCEPass::ID, F);                 // 可能产生新的死代码
    // ... 其他所有数据流分析
  }
  return changed;
}

void Reg2Mem::getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
  // Reg2Mem 通常不需要特定的分析作为依赖，因为它主要是一个转换。
  // 但它会使许多分析失效。
  analysisInvalidations.insert(&LivenessAnalysisPass::ID); // 例如
  analysisInvalidations.insert(&DominatorTreeAnalysisPass::ID);
}

} // namespace sysy