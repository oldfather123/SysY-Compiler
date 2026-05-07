#include "../include/midend/FuncInline.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include "../include/frontend/IR.h"
#include "../include/midend/FuncAnalysis.h"

/**
 * @file FuncInline.cpp
 *
 * 定义了函数内联的源文件
 */
namespace sysy {
/**
 * @brief 对函数运行函数内联
 *
 * @param [in] func 函数
 * @return 无返回值
 */
auto FuncInline::myinline(Function *func) -> void {
  auto blocklist = getBlockListBefore(func);
  for (auto callerBlock : blocklist) {
    auto iter = std::find_if(callerBlock->getInstructions().rbegin(), callerBlock->getInstructions().rend(),
                             [](const auto &instr) { return instr->isCall(); });
    while (iter != callerBlock->getInstructions().rend()) {
      auto callInstr = dynamic_cast<CallInst *>(iter->get());
      auto callee = callInstr->getCallee();
      if (pModule->getExternalFunctions().count(callee->getName()) == 0U && callee != func &&
          (FuncAnalysis::getFuncCodeSize(func) <= below || FuncAnalysis::getFuncCodeSize(callee) <= threshold ||
           (callInstr->getParent()->getParent()->getLoopOfBasicBlock(callInstr->getParent()) != nullptr &&
            (callee->getAttribute() & Function::FunctionAttribute::SelfRecursive) == 0U))) {
        // 从callInstr处分块为B1和B2
        auto generatedBlock = func->addBasicBlock("L_" + std::to_string(getcpLabelIndex()));
        generatedBlock->getInstructions().splice(generatedBlock->begin(), callerBlock->getInstructions(), iter.base(),
                                                 callerBlock->getInstructions().end());
        // 将callerBlock的所有后继结点的前驱指向generatedBlock，并将这些后继结点全部移动到generatedBlock中
        for (auto succIt = callerBlock->getSuccessors().begin(); succIt != callerBlock->getSuccessors().end();) {
          auto succ = *succIt;
          succ->replacePredecessor(callerBlock, generatedBlock);
          generatedBlock->addSuccessor(succ);
          succIt = callerBlock->getSuccessors().erase(succIt);
        }
        for (auto &generatedInstr : generatedBlock->getInstructions()) {
          generatedInstr->setParent(generatedBlock);
        }

        // 复制callee的basicblock和instruction
        auto calleeFuncClone = callee->clone();
        record.emplace_back(calleeFuncClone);
        for (const auto &block : calleeFuncClone->getBasicBlocks()) {
          block->setName("L_" + std::to_string(getcpLabelIndex()));
          block->setParent(func);
        }

        // 从entry块的前args.size()后对每个alloca开始store参数，并给复制的CFG的Entry加上前驱B1
        auto arguments = callInstr->getArguments();
        std::vector<Value *> args;
        for (const auto &arg : arguments) {
          args.push_back(arg->getValue());
        }
        const auto &calleeArgs = calleeFuncClone->getEntryBlock()->getArguments();

        // store args[i] to calleeArgs[i], not consider array so far.
        for (auto i = 0U; i < arguments.size(); ++i) {
          auto argI = args[i];
          auto argAlloca = calleeArgs[i];
          auto it =
              std::find_if(calleeFuncClone->getEntryBlock()->getInstructions().begin(),
                           calleeFuncClone->getEntryBlock()->getInstructions().end(), [argAlloca](const auto &instr) {
                             return instr->isAlloca() && dynamic_cast<AllocaInst *>(instr.get()) == argAlloca;
                           });
          assert(it != calleeFuncClone->getEntryBlock()->getInstructions().end());
          if (isSubArray(argI)) {
            argI->replaceAllUsesWith(argAlloca);
            auto getSubArrayInst = dynamic_cast<LVal *>(argI)->getDefineInst();
            dynamic_cast<LVal *>(argI)->getFatherLVal()->addChild(argAlloca);
            argAlloca->setFatherLVal(dynamic_cast<LVal *>(argI)->getFatherLVal());
            usedelete(dynamic_cast<Instruction *>(argI));
            dynamic_cast<LVal *>(argI)->getFatherLVal()->removeChild(dynamic_cast<LVal *>(argI));
            static_cast<void>(it->release());
            calleeFuncClone->getEntryBlock()->getInstructions().erase(it);
            argAlloca->setDefineInst(getSubArrayInst);
          } else {
            pBuilder->setPosition(calleeFuncClone->getEntryBlock(), std::next(it));
            pBuilder->createStoreInst(argI, argAlloca, {});
          }
        }

        calleeFuncClone->getEntryBlock()->getArguments().clear();
        callerBlock->addSuccessor(calleeFuncClone->getEntryBlock());
        calleeFuncClone->getEntryBlock()->addPredecessor(callerBlock);

        // 给复制的CFG所有无后继结点增加后继B2，并替换返回值
        // 先创建一个alloc用于接收返回值
        if (calleeFuncClone->getReturnType()->isInt() || calleeFuncClone->getReturnType()->isFloat()) {
          pBuilder->setPosition(callerBlock, callerBlock->getInstructions().end());
          auto newalloca = pBuilder->createAllocaInst(
              Type::getPointerType(calleeFuncClone->getReturnType()), {},
              calleeFuncClone->getName() + "_ret_" + std::to_string(getcpAllocaIndex()) + "@");
          auto retbbs = getReturnBlocks(calleeFuncClone);
          for (const auto retbb : retbbs) {
            auto &retInstr = retbb->getInstructions().back();
            auto retInst = dynamic_cast<ReturnInst *>(retInstr.get());
            auto retValue = retInst->getReturnValue();
            pBuilder->setPosition(retbb, retbb->getInstructions().end());
            pBuilder->createStoreInst(retValue, newalloca, {});
            usedelete(std::prev(std::prev(retbb->getInstructions().end()))->get());
            retbb->getInstructions().erase(std::prev(std::prev(retbb->getInstructions().end())));

            pBuilder->setPosition(retbb, retbb->getInstructions().end());
            pBuilder->createUncondBrInst(generatedBlock, {});
            retbb->addSuccessor(generatedBlock);
            generatedBlock->addPredecessor(retbb);
          }
          pBuilder->setPosition(generatedBlock, generatedBlock->getInstructions().begin());
          auto newload = pBuilder->createLoadInst(newalloca, {}, "%_" + std::to_string(getcpTmpIndex()));
          callInstr->replaceAllUsesWith(newload);

          usedelete(callInstr);
          callerBlock->getInstructions().erase(std::prev(std::prev(callerBlock->getInstructions().end())));
        } else {
          auto retbbs = getReturnBlocks(calleeFuncClone);
          for (const auto retbb : retbbs) {
            usedelete(retbb->getInstructions().back().get());
            retbb->getInstructions().pop_back();

            pBuilder->setPosition(retbb, retbb->getInstructions().end());
            pBuilder->createUncondBrInst(generatedBlock, {});
            retbb->addSuccessor(generatedBlock);
            generatedBlock->addPredecessor(retbb);
          }
          usedelete(callInstr);
          callerBlock->getInstructions().pop_back();
        }

        pBuilder->setPosition(callerBlock, callerBlock->getInstructions().end());
        pBuilder->createUncondBrInst(calleeFuncClone->getEntryBlock(), {});

        // 将calleefuncclone中的所有unique_ptr blocks移动到func的basicblocks中
        func->getBasicBlocks_NoRange().splice(
            func->getBasicBlocks_NoRange().end(), calleeFuncClone->getBasicBlocks_NoRange(),
            calleeFuncClone->getBasicBlocks_NoRange().begin(), calleeFuncClone->getBasicBlocks_NoRange().end());

        iter = std::find_if(callerBlock->getInstructions().rbegin(), callerBlock->getInstructions().rend(),
                            [](const auto &instr) { return instr->isCall(); });
      } else {
        iter = std::find_if(std::next(iter), callerBlock->getInstructions().rend(),
                            [](const auto &instr) { return instr->isCall(); });
      }
    }
  }
}

/**
 * @brief 运行函数内联
 *
 * @param [in] void
 * @return 无返回值
 */
auto FuncInline::run() -> void {
  // tragedy 1:
  // for (const auto &functions : pModule->getFunctions()) {
  //   const auto &[name, func] = functions;
  //   if (name == "main") {
  //     continue;
  //   }
  //   myinline(func.get());
  // }
  // auto mainFunc = pModule->getFunction("main");
  // myinline(mainFunc);

  // tragedy 2:
  auto CG = FuncAnalysis::buildCallGraph(pModule);
  auto sorted = FuncAnalysis::topologocalSort(CG);
  for (auto iter = sorted.rbegin(); iter != sorted.rend(); ++iter) {
    auto func = *iter;
    myinline(func);
  }
}

/**
 * @brief 获取运行inline之前函数的所有块
 *
 * @param [in] func 函数
 * @return 函数的所有块
 */
auto FuncInline::getBlockListBefore(Function *func) -> std::list<BasicBlock *> {
  std::list<BasicBlock *> blockList;
  for (const auto &block : func->getBasicBlocks()) {
    blockList.push_back(block.get());
  }
  return blockList;
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto FuncInline::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    val1->removeUse(use1);
  }
}

/**
 * @brief 获取函数的返回块
 *
 * @param [in] func 函数
 * @return 函数的返回块集合
 */
auto FuncInline::getReturnBlocks(Function *func) -> std::set<BasicBlock *> {
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
 * @brief 判断是否是数组
 *
 * @param [in] argI 值
 * @return 是否是子数组
 */
auto FuncInline::isSubArray(Value *argI) -> bool {
  auto lval = dynamic_cast<LVal *>(argI);
  return lval != nullptr && lval->getFatherLVal() != nullptr;
}
}  // namespace sysy
