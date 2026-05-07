#include "../include/midend/DFE.h"
#include "../include/frontend/IR.h"

/**
 * @file DFE.cpp
 *
 * 定义了DFE的源文件
 */

namespace sysy {
/**
 * @brief 运行DFE
 *
 * 无参数也无返回值，运行DFE
 *
 * @param [in] void
 * @return 无返回值
 */
auto DFE::run() -> void {
  auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    const auto &func = function.second;
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &block : func->getBasicBlocks()) {
        auto &instrs = block->getInstructions();
        for (auto iter = instrs.begin(); iter != instrs.end();) {
          auto inst = iter->get();
          if (inst->isCall()) {
            auto callInst = dynamic_cast<CallInst *>(inst);
            auto callee = callInst->getCallee();
            if (pModule->getExternalFunctions().count(callee->getName()) == 0U &&
                (callee->getAttribute() & Function::FunctionAttribute::SideEffect) == 0U &&
                (callee->getReturnType()->isVoid() || callInst->getUses().empty())) {
              changed = true;
              usedelete(inst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else {
            ++iter;
          }
        }
      }
    }
  }
  for (auto iter = functions.begin(); iter != functions.end();) {
    const auto &[name, func] = *iter;
    if (func->getBasicBlocks().empty() || (name != "main" && func->getUses().empty()) ||
        (name != "main" && isUsedOnlyByOneFunc(func.get()) &&
         (func->getAttribute() & Function::FunctionAttribute::SelfRecursive) != 0)) {
      for (auto &bb : func->getBasicBlocks()) {
        for (auto &inst : bb->getInstructions()) {
          usedelete(inst.get());
        }
      }
      iter = functions.erase(iter);
    } else {
      ++iter;
    }
  }
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto DFE::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    val1->removeUse(use1);
  }
}

/**
 * @brief 判断函数是否只被一个函数调用
 *
 * @param [in] func 函数
 * @return 是否只被一个函数调用
 */
auto DFE::isUsedOnlyByOneFunc(Function *func) -> bool {
  auto indentical = dynamic_cast<Instruction *>(func->getUses().front()->getUser())->getParent()->getParent();
  return std::all_of(func->getUses().begin(), func->getUses().end(), [indentical](const auto &use) {
    return dynamic_cast<Instruction *>(use->getUser())->getParent()->getParent() == indentical;
  });
}

}  // namespace sysy
