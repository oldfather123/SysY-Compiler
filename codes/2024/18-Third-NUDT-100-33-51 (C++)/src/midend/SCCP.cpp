#include "../include/midend/SCCP.h"
#include <variant>
#include "../include/frontend/IR.h"

/**
 * @file SCCP.cpp
 *
 * 定义了SCCP的源文件
 */

namespace sysy {
/**
 * @brief 稀疏条件常量传播
 *
 * 无参数也无返回值，运行稀疏条件常量传播
 *
 * @param [in] void
 * @return 无返回值
 */
auto SCCP::run() -> void {
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
            auto value = storeInst->getValue();
            auto pointer = storeInst->getPointer();
            auto constant = dynamic_cast<ConstantValue *>(value);
            if (isArr(pointer) || isGlobal(pointer) || constant == nullptr) {
              ++iter;
              continue;
            }
            pointer->replaceAllUsesWith(constant);
            changed = true;
            usedelete(storeInst);
            iter = instrs.erase(iter);
          } else if (inst->isLoad()) {
            auto loadInst = dynamic_cast<LoadInst *>(inst);
            auto pointer = loadInst->getPointer();
            auto constant = dynamic_cast<ConstantValue *>(pointer);
            if (isArr(pointer) || isGlobal(pointer) || constant == nullptr) {
              ++iter;
              continue;
            }
            loadInst->replaceAllUsesWith(constant);
            changed = true;
            usedelete(loadInst);
            iter = instrs.erase(iter);
          } else if (inst->isBinary()) {
            auto binInst = dynamic_cast<BinaryInst *>(inst);
            auto lhs = binInst->getLhs();
            auto rhs = binInst->getRhs();
            auto constant1 = dynamic_cast<ConstantValue *>(lhs);
            auto constant2 = dynamic_cast<ConstantValue *>(rhs);
            if (constant1 == nullptr || constant2 == nullptr) {
              ++iter;
              continue;
            }
            std::variant<int, float> value;
            if (constant1->isInt() && constant2->isInt()) {
              value = binInst->eval<int>(constant1->getValue<int>(), constant2->getValue<int>());
            } else {
              value = binInst->eval<float>(constant1->getValue<float>(), constant2->getValue<float>());
            }
            auto constant = std::visit([](auto &&arg) -> ConstantValue * { return ConstantValue::get(arg); }, value);
            binInst->replaceAllUsesWith(constant);
            changed = true;
            usedelete(binInst);
            iter = instrs.erase(iter);
          } else if (inst->isUnary()) {
            auto unInst = dynamic_cast<UnaryInst *>(inst);
            auto operand = unInst->getOperand();
            auto constant1 = dynamic_cast<ConstantValue *>(operand);
            if (constant1 == nullptr) {
              ++iter;
              continue;
            }
            std::variant<int, float> value;
            ConstantValue *constant = nullptr;
            switch (unInst->getKind()) {
              case UnaryInst::kNeg:
                value = -constant1->getValue<int>();
                constant = ConstantValue::get(std::get<int>(value));
                break;
              case UnaryInst::kNot:
                value = constant1->getValue<int>() == 0;
                constant = ConstantValue::get(std::get<int>(value));
                break;
              case UnaryInst::kFNeg:
                value = -constant1->getValue<float>();
                constant = ConstantValue::get(std::get<float>(value));
                break;
              case UnaryInst::kFNot:
                value = static_cast<float>(constant1->getValue<float>() == 0.0F);
                constant = ConstantValue::get(std::get<float>(value));
                break;
              case UnaryInst::kFtoI:
                value = static_cast<int>(constant1->getValue<float>());
                constant = ConstantValue::get(std::get<int>(value));
                break;
              case UnaryInst::kItoF:
                value = static_cast<float>(constant1->getValue<int>());
                constant = ConstantValue::get(std::get<float>(value));
                break;
              default:
                break;
            }
            unInst->replaceAllUsesWith(constant);
            changed = true;
            usedelete(unInst);
            iter = instrs.erase(iter);
          } else if (inst->isAlloca()) {
            auto allocaInst = dynamic_cast<AllocaInst *>(inst);
            if (!allocaInst->getUses().empty() ||
                std::find(func->getEntryBlock()->getArguments().begin(), func->getEntryBlock()->getArguments().end(),
                          allocaInst) != func->getEntryBlock()->getArguments().end()) {
              ++iter;
              continue;
            }
            changed = true;
            usedelete(allocaInst);
            iter = instrs.erase(iter);
          } else if (inst->isPhi()) {
            auto phiInst = dynamic_cast<PhiInst *>(inst);
            auto pointer = phiInst->getPointer();
            auto values = phiInst->getValues();
            bool tag = true;
            bool tag1 = false;
            ConstantValue *constant = nullptr;
            for (auto &value : values) {
              if (value->getValue() == pointer) {
                continue;
              }
              if (!tag1) {
                tag1 = true;
                constant = dynamic_cast<ConstantValue *>(value->getValue());
              }
              auto temp = dynamic_cast<ConstantValue *>(value->getValue());
              if (temp == nullptr || temp != constant) {
                tag = false;
                break;
              }
            }
            if (!tag) {
              ++iter;
              continue;
            }
            pointer->replaceAllUsesWith(constant);
            changed = true;
            usedelete(phiInst);
            iter = instrs.erase(iter);
          } else if (inst->isCall()) {
            auto callInst = dynamic_cast<CallInst *>(inst);
            auto callee = callInst->getCallee();
            if (pModule->getExternalFunctions().count(callee->getName()) != 0 ||
                (!callee->getReturnType()->isInt() && !callee->getReturnType()->isFloat())) {
              ++iter;
              continue;
            }
            bool tag = false;
            bool tag1 = true;
            ConstantValue *replace = nullptr;
            for (const auto &cb : callee->getBasicBlocks()) {
              assert(cb->getInstructions().back());
              if (cb->getInstructions().back()->isReturn()) {
                auto retInst = dynamic_cast<ReturnInst *>(cb->getInstructions().back().get());
                auto value = retInst->getReturnValue();
                auto constant = dynamic_cast<ConstantValue *>(value);
                if (constant == nullptr) {
                  tag1 = false;
                  break;
                }
                if (!tag) {
                  tag = true;
                  replace = constant;
                } else if (replace != constant) {
                  tag1 = false;
                  break;
                }
              }
            }
            if (tag1 && replace != nullptr) {
              callInst->replaceAllUsesWith(replace);
              // changed = true;
              if ((callee->getAttribute() & Function::FunctionAttribute::SideEffect) == 0) {
                usedelete(callInst);
                iter = instrs.erase(iter);
              } else {
                ++iter;
                continue;
              }
            } else {
              ++iter;
              continue;
            }
          } else {
            // for other instructions, such as br, ret, la, memset
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
    }
  }
}

/**
 * @brief 判断一个value是不是全局变量
 *
 * @param [in] val 一个value
 * @return 返回true表示是全局变量，返回false表示不是
 */
auto SCCP::isGlobal(Value *val) -> bool {
  auto gval = dynamic_cast<GlobalValue *>(val);
  return gval != nullptr;
}

/**
 * @brief 判断一个value是不是数组
 *
 * @param [in] val 一个value
 * @return 返回true表示是数组，返回false表示不是
 */
auto SCCP::isArr(Value *val) -> bool {
  auto aval = dynamic_cast<AllocaInst *>(val);
  return aval != nullptr && aval->getNumDims() != 0;
}

/**
 * @brief 删除一个指令的operand对应的value的该条use
 *
 * @param [in] inst 一条指令
 * @return 无返回值
 */
auto SCCP::usedelete(Instruction *instr) -> void {
  for (auto &use : instr->getOperands()) {
    auto val = use->getValue();
    val->removeUse(use);
  }
}
}  // namespace sysy
