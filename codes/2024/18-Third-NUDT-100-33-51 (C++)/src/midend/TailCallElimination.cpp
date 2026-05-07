#include "../include/midend/TailCallElimination.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <vector>
#include "../include/frontend/IR.h"

/**
 * @file TailCallElimination.cpp
 *
 * 定义了尾调用消除的源文件
 */
namespace sysy {
/**
 * @brief 运行尾调用消除
 *
 * @return 无返回值
 */
auto TailCallElimination::run() -> void {
  for (const auto &functions : pModule->getFunctions()) {
    const auto &[name, func] = functions;
    if ((func->getAttribute() & Function::FunctionAttribute::SelfRecursive) == 0) {
      continue;
    }
    auto returnBlocks = getReturnBlocks(func.get());
    for (auto block : returnBlocks) {
      auto retInstr = dynamic_cast<ReturnInst *>(block->getInstructions().back().get());
      assert(retInstr);
      auto retVal = retInstr->getReturnValue();
      // void先等等哈
      if (retVal == nullptr) {
        continue;
      }
      auto callInstr = dynamic_cast<CallInst *>(retVal);
      // 不是call指令先不处理
      if (callInstr == nullptr) {
        continue;
      }
      // 不是尾调用自己先不处理
      auto callee = callInstr->getCallee();
      if (callee != func.get()) {
        continue;
      }

      auto entryBlock = func->getEntryBlock();
      auto funcArgs = entryBlock->getArguments();
      // 如果参数中有数组，先不处理
      if (hasArray(funcArgs)) {
        continue;
      }

      // 尾调用消除
      usedelete(retInstr);
      block->getInstructions().pop_back();

      auto args = callInstr->getArguments();
      std::vector<Value *> callArgs;
      for (const auto &arg : args) {
        callArgs.push_back(arg->getValue());
      }
      pBuilder->setPosition(block, block->getInstructions().end());
      for (size_t i = 0; i < funcArgs.size(); ++i) {
        pBuilder->createStoreInst(callArgs[i], funcArgs[i], {});
      }

      auto iter = block->getInstructions().end();
      for (size_t i = 0; i < funcArgs.size(); ++i) {
        iter = std::prev(iter);
      }
      iter = std::prev(iter);
      usedelete(iter->get());
      block->getInstructions().erase(iter);

      iter = std::find_if(entryBlock->getInstructions().begin(), entryBlock->getInstructions().end(),
                          [&funcArgs](const auto &instr) { return instr.get() == funcArgs.back(); });
      iter = std::next(iter);
      auto generatedBlock = func->addBasicBlock("L_TailEntry");
      generatedBlock->getInstructions().splice(generatedBlock->begin(), entryBlock->getInstructions(), iter,
                                               entryBlock->getInstructions().end());
      for (auto succIt = entryBlock->getSuccessors().begin(); succIt != entryBlock->getSuccessors().end();) {
        auto succ = *succIt;
        succ->replacePredecessor(entryBlock, generatedBlock);
        generatedBlock->addSuccessor(succ);
        succIt = entryBlock->getSuccessors().erase(succIt);
      }
      for (auto &generatedInstr : generatedBlock->getInstructions()) {
        generatedInstr->setParent(generatedBlock);
      }
      entryBlock->addSuccessor(generatedBlock);
      generatedBlock->addPredecessor(entryBlock);

      pBuilder->setPosition(entryBlock, entryBlock->getInstructions().end());
      pBuilder->createUncondBrInst(generatedBlock, {});

      pBuilder->setPosition(block, block->getInstructions().end());
      pBuilder->createUncondBrInst(generatedBlock, {});

      block->addSuccessor(generatedBlock);
      generatedBlock->addPredecessor(block);
    }
  }
}

/**
 * @brief 获取函数的返回块
 *
 * @param [in] func 函数
 * @return 返回函数的返回块
 */
auto TailCallElimination::getReturnBlocks(Function *func) -> std::set<BasicBlock *> {
  std::set<BasicBlock *> returnBlocks;
  for (const auto &block : func->getBasicBlocks()) {
    assert(block->getInstructions().back());
    if (block->getInstructions().back()->isReturn()) {
      returnBlocks.insert(block.get());
    }
  }
  return returnBlocks;
}

/**
 * @brief 删除指令相关的value-use-user关系
 *
 * @param [in] instr 指令
 * @return 无返回值
 */
auto TailCallElimination::usedelete(Instruction *instr) -> void {
  for (auto &use : instr->getOperands()) {
    auto val = use->getValue();
    val->removeUse(use);
  }
}

/**
 * @brief 判断是否是数组
 *
 * @param [in] argVec 参数列表
 * @return 是否是数组
 */
auto TailCallElimination::hasArray(const std::vector<AllocaInst *> &argVec) -> bool {
  return std::any_of(argVec.begin(), argVec.end(), [this](const auto &arg) { return isArr(arg); });
}

/**
 * @brief 判断一个value是不是数组
 *
 * @param [in] val 一个value
 * @return 返回true表示是数组，返回false表示不是
 */
auto TailCallElimination::isArr(Value *val) -> bool {
  auto aval = dynamic_cast<AllocaInst *>(val);
  return aval != nullptr && aval->getNumDims() != 0;
}
}  // namespace sysy
