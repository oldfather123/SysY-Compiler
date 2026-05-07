#include "../include/midend/Global2Local.h"
#include <algorithm>
#include "../include/frontend/IR.h"

/**
 * @file Global2Local.cpp
 *
 * 定义了Global2Local的源文件
 */

namespace sysy {
/**
 * @brief 运行G2L
 *
 * @note 这里只考虑scalar的全局变量的G2L
 *
 * @param [in] void
 * @return 无返回值
 */
auto Global2Local::run() -> void {
  calculateg2f();
  auto &globals = pModule->getGlobals();
  for (auto iter = globals.begin(); iter != globals.end();) {
    auto &global = *iter;
    // wait，先不考虑全局数组，目前只做标量的g2l
    if (global->getDims().size() != 0) {
      ++iter;
      continue;
    }
    // 1. 如果全局变量只在一个函数中使用
    // 2. 且该函数是main || 该函数的attribute不是selfrecursive的，且该函数只会被调用一次，且调用点不在循环内
    // 3. 且该全局变量没有被La指令使用（也就是说不是数组作为调用一个函数的参数）--> wait，先不考虑全局数组
    // 则将其转化为局部变量
    if (global2func[global.get()].size() == 1 &&
        (((*global2func[global.get()].begin())->getName() == "main") ||
         (((*global2func[global.get()].begin())->getAttribute() & Function::FunctionAttribute::SelfRecursive) == 0 &&
          (*global2func[global.get()].begin())->getUses().size() == 1 &&
          dynamic_cast<Instruction *>((*global2func[global.get()].begin())->getUses().front()->getUser())
                  ->getParent()
                  ->getParent()
                  ->getLoopOfBasicBlock(
                      dynamic_cast<Instruction *>((*global2func[global.get()].begin())->getUses().front()->getUser())
                          ->getParent()) == nullptr))) {
      auto func = *global2func[global.get()].begin();
      auto entry = func->getEntryBlock();
      pBuilder->setPosition(entry, entry->begin());
      std::vector<Value *> dims;
      for (const auto &dim : global->getDims()) {
        dims.push_back(dim->getValue());
      }
      auto newalloca = pBuilder->createAllocaInst(global->getType(), dims, global->getName());
      auto init = global->getInitValues();
      auto &valueList = init.getValues();
      // auto &numList = init.getNumbers();
      if (dims.empty()) {
        pBuilder->createStoreInst(valueList[0], newalloca, {});
      }
      // else {
      //   int begin = 0;
      //   for (unsigned i = 0; i < numList.size(); i++) {
      //     auto value = valueList[i];
      //     int num = numList[i];
      //     pBuilder->createMemsetInst(newalloca, ConstantValue::get(begin), ConstantValue::get(num), value);
      //     begin += num;
      //   }
      // }
      global->replaceAllUsesWith(newalloca);
      global2func.erase(global.get());
      iter = globals.erase(iter);
    } else {
      ++iter;
    }
  }
}

/**
 * @brief 计算全局变量对应的函数
 *
 * @param [in] void
 * @return 无返回值
 */
auto Global2Local::calculateg2f() -> void {
  global2func.clear();
  const auto &globals = pModule->getGlobals();
  for (const auto &global : globals) {
    for (const auto &use : global->getUses()) {
      auto user = use->getUser();
      auto inst = dynamic_cast<Instruction *>(user);
      if (inst != nullptr) {
        global2func[global.get()].insert(inst->getParent()->getParent());
      }
    }
  }
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto Global2Local::usedelete(Instruction *instr) -> void {
  for (auto &use1 : instr->getOperands()) {
    auto val1 = use1->getValue();
    val1->removeUse(use1);
  }
}

/**
 * @brief 判断全局变量是否被GetSubArray使用
 *
 * @param [in] global 全局变量
 * @return 返回true表示被GetSubArray使用，返回false表示没有被GetSubArray使用
 */
auto Global2Local::GisUsedByGetSubArray(GlobalValue *global) -> bool {
  return std::any_of(global->getUses().begin(), global->getUses().end(), [](const std::shared_ptr<sysy::Use> &usePtr) {
    auto user = usePtr->getUser();
    auto inst = dynamic_cast<Instruction *>(user);
    return inst != nullptr && inst->isGetSubArray();
  });
}
}  // namespace sysy
