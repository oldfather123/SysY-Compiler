#include "../include/midend/DCE.h"
#include <algorithm>
#include "../include/frontend/IR.h"

/**
 * @file DCE.cpp
 *
 * 定义了DCE的源文件
 */
namespace sysy {
/**
 * @brief 死代码删除
 *
 * 无参数也无返回值，运行死代码删除
 *
 * @param [in] void
 * @return 无返回值
 */
auto DCE::run() -> void {
  const auto &functions = pModule->getFunctions();
  for (const auto &function : functions) {
    const auto &func = function.second;
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &block : func->getBasicBlocks()) {
        auto &instrs = block->getInstructions();
        for (auto iter = instrs.begin(); iter != instrs.end();) {
          auto inst = iter->get();
          if (inst->isStore()) {
            auto storeInst = dynamic_cast<StoreInst *>(inst);
            auto pointer = storeInst->getPointer();
            if (isGlobal(pointer) ||
                (isArr(pointer) &&
                 std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                           pointer) != func->getEntryBlock()->getArguments().end())) {
              ++iter;
              continue;
            }
            bool tag = true;
            for (auto &use : pointer->getUses()) {
              auto user = use->getUser();
              auto userInst = dynamic_cast<Instruction *>(user);
              if (userInst != nullptr && !userInst->isAlloca() && !userInst->isStore()) {
                tag = false;
                break;
              }
            }
            if (tag) {
              changed = true;
              usedelete(storeInst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else if (inst->isBinary() || inst->isUnary() || inst->isLoad()) {
            if (inst->getUses().empty()) {
              changed = true;
              usedelete(inst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else if (inst->isAlloca()) {
            auto allocaInst = dynamic_cast<AllocaInst *>(inst);
            if (allocaInst->getUses().empty() &&
                std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                          allocaInst) == func->getEntryBlock()->getArguments().end()) {
              changed = true;
              usedelete(inst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else if (inst->isPhi()) {
            auto phiInst = dynamic_cast<PhiInst *>(inst);
            auto pointer = phiInst->getPointer();
            bool tag = true;
            for (const auto &use : pointer->getUses()) {
              auto user = use->getUser();
              if (user != inst) {
                tag = false;
                break;
              }
            }
            if (tag) {
              changed = true;
              usedelete(inst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else if (inst->isMemset()) {
            auto memsetInst = dynamic_cast<MemsetInst *>(inst);
            auto pointer = memsetInst->getPointer();
            if (pointer->getUses().empty()) {
              changed = true;
              usedelete(inst);
              iter = instrs.erase(iter);
            } else {
              ++iter;
            }
          } else {
            // for other instructions, such as call, condbr, br, return
            // call elimination is considered in DFE
            ++iter;
          }
        }
      }
      // 删除mem2reg时引入的且现在已经没有value使用到它的了的隐式alloca
      for (auto it = func->getIndirectAllocas().begin(); it != func->getIndirectAllocas().end();) {
        auto &allocaInst = *it;
        if (allocaInst->getUses().empty()) {
          changed = true;
          it = func->getIndirectAllocas().erase(it);
        } else {
          ++it;
        }
      }
      // 删除未使用的全局变量
      auto &globals = pModule->getGlobals();
      for (auto it = globals.begin(); it != globals.end();) {
        auto &global = *it;
        if (global->getUses().empty()) {
          changed = true;
          it = globals.erase(it);
        } else {
          ++it;
        }
      }
    }
  }
}

/**
 * @brief 判断一个value是不是全局变量
 *
 * @param [in] val 一个value
 * @return 返回true表示是全局变量，返回false表示不是
 */
auto DCE::isGlobal(Value *val) -> bool {
  auto gval = dynamic_cast<GlobalValue *>(val);
  return gval != nullptr;
}

/**
 * @brief 判断一个value是不是数组
 *
 * @param [in] val 一个value
 * @return 返回true表示是数组，返回false表示不是
 */
auto DCE::isArr(Value *val) -> bool {
  auto aval = dynamic_cast<AllocaInst *>(val);
  return aval != nullptr && aval->getNumDims() != 0;
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto DCE::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    val1->removeUse(use1);
  }
}
}  // namespace sysy
