#include "../include/frontend/IR.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <vector>
#include "../include/frontend/IRBuilder.h"

/**
 * @file IR.cpp
 *
 * @brief 定义IR相关类型与操作的源文件
 */
namespace sysy {

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//
/**
 * @brief 获取int类型的指针
 *
 * @return int类型指针
 */
auto Type::getIntType() -> Type * {
  static Type intType(kInt);
  return &intType;
}
/**
 * @brief 获取float类型的指针
 *
 * @return float类型指针
 */
auto Type::getFloatType() -> Type * {
  static Type floatType(kFloat);
  return &floatType;
}
/**
 * @brief 获取void类型的指针
 *
 * @return void类型指针
 */
auto Type::getVoidType() -> Type * {
  static Type voidType(kVoid);
  return &voidType;
}
/**
 * @brief 获取label类型的指针
 *
 * @return label类型指针
 */
auto Type::getLabelType() -> Type * {
  static Type labelType(kLabel);
  return &labelType;
}
/**
 * @brief 获取pointer类型的指针
 *
 * @param [in] baseType pointer所指向的类型指针
 * @return pointer类型指针
 */
auto Type::getPointerType(Type *baseType) -> Type * {
  // forward to PointerType
  return PointerType::get(baseType);
}
/**
 * @brief 获取function类型的指针
 *
 * @param [in] returnType funciton类型的返回值类型
 * @param [in] paramTypes function类型的形式参数类型指针列表
 * @return function类型指针
 */
auto Type::getFunctionType(Type *returnType, const std::vector<Type *> &paramTypes) -> Type * {
  // forward to FunctionType
  return FunctionType::get(returnType, paramTypes);
}
/**
 * @brief 获取类型所占大小
 *
 * @return 类型所占大小(以字节计)
 */
auto Type::getSize() const -> unsigned {
  switch (kind) {
    case kInt:
    case kFloat:
      return 4;
    case kLabel:
    case kPointer:
    case kFunction:
      return 8;
    case kVoid:
      return 0;
  }
  return 0;
}

/**
 * @brief 获取指向特定类型的pointer类型
 *
 * @param [in] baseType pointer所指向的类型指针
 * @return pointer类型指针
 */
auto PointerType::get(Type *baseType) -> PointerType * {
  static std::map<Type *, std::unique_ptr<PointerType>> pointerTypes;
  auto iter = pointerTypes.find(baseType);
  if (iter != pointerTypes.end()) {
    return iter->second.get();
  }
  auto type = new PointerType(baseType);
  assert(type);
  auto result = pointerTypes.emplace(baseType, type);
  return result.first->second.get();
}
/**
 * @brief 获取特定返回值类型和形式参数类型的function类型的指针
 *
 * @param [in] returnType funciton类型的返回值类型
 * @param [in] paramTypes function类型的形式参数类型指针列表
 * @return function类型指针
 */
auto FunctionType::get(Type *returnType, const std::vector<Type *> &paramTypes) -> FunctionType * {
  static std::set<std::unique_ptr<FunctionType>> functionTypes;
  auto iter =
      std::find_if(functionTypes.begin(), functionTypes.end(), [&](const std::unique_ptr<FunctionType> &type) -> bool {
        if (returnType != type->getReturnType() ||
            paramTypes.size() != static_cast<size_t>(type->getParamTypes().size())) {
          return false;
        }
        return std::equal(paramTypes.begin(), paramTypes.end(), type->getParamTypes().begin());
      });
  if (iter != functionTypes.end()) {
    return iter->get();
  }
  auto type = new FunctionType(returnType, paramTypes);
  assert(type);
  auto result = functionTypes.emplace(type);
  return result.first->get();
}
/**
 * @brief 替换所有use关系中的value
 *
 * @param [in] value 所要替换成的value
 * @return 无返回值
 */
void Value::replaceAllUsesWith(Value *value) {
  for (auto &use : uses) {
    use->getUser()->setOperand(use->getIndex(), value);
  }
  uses.clear();
}
/**
 * @brief 获取整型常量对应的字面值指针
 *
 * @param [in] value 整型常量
 * @return 字面值指针
 */
auto ConstantValue::get(int value) -> ConstantValue * {
  static std::map<int, std::unique_ptr<ConstantValue>> intConstants;
  auto iter = intConstants.find(value);
  if (iter != intConstants.end()) {
    return iter->second.get();
  }
  auto inst = new ConstantValue(value);
  assert(inst);
  auto result = intConstants.emplace(value, inst);
  return result.first->second.get();
}
/**
 * @brief 获取浮点常量对应的字面值指针
 *
 * @param [in] value 浮点常量
 * @return 字面值指针
 */
auto ConstantValue::get(float value) -> ConstantValue * {
  static std::map<float, std::unique_ptr<ConstantValue>> floatConstants;
  auto iter = floatConstants.find(value);
  if (iter != floatConstants.end()) {
    return iter->second.get();
  }
  auto inst = new ConstantValue(value);
  assert(inst);
  auto result = floatConstants.emplace(value, inst);
  return result.first->second.get();
}

auto Function::getCalleesWithNoExternalAndSelf() -> std::set<Function *> {
  std::set<Function *> result;
  for (auto callee : callees) {
    if (parent->getExternalFunctions().count(callee->getName()) == 0U && callee != this) {
      result.insert(callee);
    }
  }
  return result;
}
/**
 * @brief 复制函数
 *
 * @param [in] suffix 变量名字后面需要添加的后缀
 * @return 指向复制函数的指针
 */
auto Function::clone(const std::string &suffix) const -> Function * {
  std::stringstream ss;
  std::map<BasicBlock *, BasicBlock *> oldNewBlockMap;
  IRBuilder builder;
  auto newFunction = new Function(parent, type, name);
  newFunction->getEntryBlock()->setName(blocks.front()->getName());
  oldNewBlockMap.emplace(blocks.front().get(), newFunction->getEntryBlock());
  auto oldBlockListIter = std::next(blocks.begin());
  while (oldBlockListIter != blocks.end()) {
    auto newBlock = newFunction->addBasicBlock(oldBlockListIter->get()->getName());
    oldNewBlockMap.emplace(oldBlockListIter->get(), newBlock);
    oldBlockListIter++;
  }

  for (const auto &oldNewBlockItem : oldNewBlockMap) {
    auto oldBlock = oldNewBlockItem.first;
    auto newBlock = oldNewBlockItem.second;
    for (const auto &oldPred : oldBlock->getPredecessors()) {
      newBlock->addPredecessor(oldNewBlockMap.at(oldPred));
    }
    for (const auto &oldSucc : oldBlock->getSuccessors()) {
      newBlock->addSuccessor(oldNewBlockMap.at(oldSucc));
    }
  }

  std::map<Value *, Value *> oldNewValueMap;
  std::map<Value *, bool> isAddedToCreate;
  std::map<Value *, bool> isCreated;
  std::queue<Value *> toCreate;

  for (const auto &oldBlock : blocks) {
    for (const auto &inst : oldBlock->getInstructions()) {
      isAddedToCreate.emplace(inst.get(), false);
      isCreated.emplace(inst.get(), false);
    }
  }
  for (const auto &oldBlock : blocks) {
    for (const auto &inst : oldBlock->getInstructions()) {
      for (const auto &valueUse : inst->getOperands()) {
        auto value = valueUse->getValue();
        if (oldNewValueMap.find(value) == oldNewValueMap.end()) {
          auto oldAllocInst = dynamic_cast<AllocaInst *>(value);
          if (oldAllocInst != nullptr) {
            std::vector<Value *> dims;
            for (const auto &dim : oldAllocInst->getDims()) {
              dims.emplace_back(dim->getValue());
            }
            ss << oldAllocInst->getName() << suffix;
            auto newAllocInst =
                new AllocaInst(oldAllocInst->getType(), dims, oldNewBlockMap.at(oldAllocInst->getParent()), ss.str());
            ss.str("");
            oldNewValueMap.emplace(oldAllocInst, newAllocInst);
            if (isAddedToCreate.find(oldAllocInst) == isAddedToCreate.end()) {
              isAddedToCreate.emplace(oldAllocInst, true);
            } else {
              isAddedToCreate.at(oldAllocInst) = true;
            }
            if (isCreated.find(oldAllocInst) == isCreated.end()) {
              isCreated.emplace(oldAllocInst, true);
            } else {
              isCreated.at(oldAllocInst) = true;
            }
          }
        }
      }
      if (inst->getKind() == Instruction::kAlloca) {
        if (oldNewValueMap.find(inst.get()) == oldNewValueMap.end()) {
          auto oldAllocInst = dynamic_cast<AllocaInst *>(inst.get());
          std::vector<Value *> dims;
          for (const auto &dim : oldAllocInst->getDims()) {
            dims.emplace_back(dim->getValue());
          }
          ss << oldAllocInst->getName() << suffix;
          auto newAllocInst =
              new AllocaInst(oldAllocInst->getType(), dims, oldNewBlockMap.at(oldAllocInst->getParent()), ss.str());
          ss.str("");
          oldNewValueMap.emplace(oldAllocInst, newAllocInst);
          if (isAddedToCreate.find(oldAllocInst) == isAddedToCreate.end()) {
            isAddedToCreate.emplace(oldAllocInst, true);
          } else {
            isAddedToCreate.at(oldAllocInst) = true;
          }
          if (isCreated.find(oldAllocInst) == isCreated.end()) {
            isCreated.emplace(oldAllocInst, true);
          } else {
            isCreated.at(oldAllocInst) = true;
          }
        }
      }
    }
  }
  for (const auto &oldBlock : blocks) {
    for (const auto &inst : oldBlock->getInstructions()) {
      for (const auto &valueUse : inst->getOperands()) {
        auto value = valueUse->getValue();
        if (oldNewValueMap.find(value) == oldNewValueMap.end()) {
          auto globalValue = dynamic_cast<GlobalValue *>(value);
          auto constVariable = dynamic_cast<ConstantVariable *>(value);
          auto constantValue = dynamic_cast<ConstantValue *>(value);
          auto functionValue = dynamic_cast<Function *>(value);
          if (globalValue != nullptr || constantValue != nullptr || constVariable != nullptr ||
              functionValue != nullptr) {
            if (functionValue == this) {
              oldNewValueMap.emplace(value, newFunction);
            } else {
              oldNewValueMap.emplace(value, value);
            }
            isCreated.emplace(value, true);
            isAddedToCreate.emplace(value, true);
          }
        }
      }
    }
  }
  for (const auto &oldBlock : blocks) {
    for (const auto &inst : oldBlock->getInstructions()) {
      if (inst->getKind() != Instruction::kAlloca) {
        bool isReady = true;
        for (const auto &use : inst->getOperands()) {
          auto value = use->getValue();
          if (dynamic_cast<BasicBlock *>(value) == nullptr && !isCreated.at(value)) {
            isReady = false;
            break;
          }
        }
        if (isReady) {
          toCreate.push(inst.get());
          isAddedToCreate.at(inst.get()) = true;
        }
      }
    }
  }

  while (!toCreate.empty()) {
    auto inst = dynamic_cast<Instruction *>(toCreate.front());
    toCreate.pop();

    bool isReady = true;
    for (const auto &valueUse : inst->getOperands()) {
      auto value = dynamic_cast<Instruction *>(valueUse->getValue());
      if (value != nullptr && !isCreated.at(value)) {
        isReady = false;
        break;
      }
    }

    if (!isReady) {
      toCreate.push(inst);
      continue;
    }
    isCreated.at(inst) = true;
    switch (inst->getKind()) {
      case Instruction::kAdd:
      case Instruction::kSub:
      case Instruction::kMul:
      case Instruction::kDiv:
      case Instruction::kRem:
      case Instruction::kICmpEQ:
      case Instruction::kICmpNE:
      case Instruction::kICmpLT:
      case Instruction::kICmpGT:
      case Instruction::kICmpLE:
      case Instruction::kICmpGE:
      case Instruction::kAnd:
      case Instruction::kOr:
      case Instruction::kFAdd:
      case Instruction::kFSub:
      case Instruction::kFMul:
      case Instruction::kFDiv:
      case Instruction::kFCmpEQ:
      case Instruction::kFCmpNE:
      case Instruction::kFCmpLT:
      case Instruction::kFCmpGT:
      case Instruction::kFCmpLE:
      case Instruction::kFCmpGE: {
        auto oldBinaryInst = dynamic_cast<BinaryInst *>(inst);
        auto lhs = oldBinaryInst->getLhs();
        auto rhs = oldBinaryInst->getRhs();
        Value *newLhs;
        Value *newRhs;
        newLhs = oldNewValueMap[lhs];
        newRhs = oldNewValueMap[rhs];
        ss << oldBinaryInst->getName() << suffix;
        auto newBinaryInst = new BinaryInst(oldBinaryInst->getKind(), oldBinaryInst->getType(), newLhs, newRhs,
                                            oldNewBlockMap.at(oldBinaryInst->getParent()), ss.str());
        ss.str("");
        oldNewValueMap.emplace(oldBinaryInst, newBinaryInst);
        break;
      }

      case Instruction::kNeg:
      case Instruction::kNot:
      case Instruction::kFNeg:
      case Instruction::kFNot:
      case Instruction::kItoF:
      case Instruction::kFtoI: {
        auto oldUnaryInst = dynamic_cast<UnaryInst *>(inst);
        auto hs = oldUnaryInst->getOperand();
        Value *newHs;
        newHs = oldNewValueMap.at(hs);
        ss << oldUnaryInst->getName() << suffix;
        auto newUnaryInst = new UnaryInst(oldUnaryInst->getKind(), oldUnaryInst->getType(), newHs,
                                          oldNewBlockMap.at(oldUnaryInst->getParent()), ss.str());
        ss.str("");
        oldNewValueMap.emplace(oldUnaryInst, newUnaryInst);
        break;
      }

      case Instruction::kCall: {
        auto oldCallInst = dynamic_cast<CallInst *>(inst);
        std::vector<Value *> newArgumnts;
        for (const auto &arg : oldCallInst->getArguments()) {
          newArgumnts.emplace_back(oldNewValueMap.at(arg->getValue()));
        }

        ss << oldCallInst->getName() << suffix;
        CallInst *newCallInst;
        newCallInst =
            new CallInst(oldCallInst->getCallee(), newArgumnts, oldNewBlockMap.at(oldCallInst->getParent()), ss.str());
        ss.str("");
        // if (oldCallInst->getCallee() != this) {
        //   newCallInst = new CallInst(oldCallInst->getCallee(), newArgumnts,
        //   oldNewBlockMap.at(oldCallInst->getParent()),
        //                              oldCallInst->getName());
        // } else {
        //   newCallInst = new CallInst(newFunction, newArgumnts, oldNewBlockMap.at(oldCallInst->getParent()),
        //                              oldCallInst->getName());
        // }

        oldNewValueMap.emplace(oldCallInst, newCallInst);
        break;
      }

      case Instruction::kCondBr: {
        auto oldCondBrInst = dynamic_cast<CondBrInst *>(inst);
        auto oldCond = oldCondBrInst->getCondition();
        Value *newCond;
        newCond = oldNewValueMap.at(oldCond);
        auto newCondBrInst = new CondBrInst(newCond, oldNewBlockMap.at(oldCondBrInst->getThenBlock()),
                                            oldNewBlockMap.at(oldCondBrInst->getElseBlock()), {}, {},
                                            oldNewBlockMap.at(oldCondBrInst->getParent()));
        oldNewValueMap.emplace(oldCondBrInst, newCondBrInst);
        break;
      }

      case Instruction::kBr: {
        auto oldBrInst = dynamic_cast<UncondBrInst *>(inst);
        auto newBrInst =
            new UncondBrInst(oldNewBlockMap.at(oldBrInst->getBlock()), {}, oldNewBlockMap.at(oldBrInst->getParent()));
        oldNewValueMap.emplace(oldBrInst, newBrInst);
        break;
      }

      case Instruction::kReturn: {
        auto oldReturnInst = dynamic_cast<ReturnInst *>(inst);
        auto oldRval = oldReturnInst->getReturnValue();
        Value *newRval = nullptr;
        if (oldRval != nullptr) {
          newRval = oldNewValueMap.at(oldRval);
        }
        auto newReturnInst =
            new ReturnInst(newRval, oldNewBlockMap.at(oldReturnInst->getParent()), oldReturnInst->getName());
        oldNewValueMap.emplace(oldReturnInst, newReturnInst);
        break;
      }

      case Instruction::kAlloca: {
        assert(false);
      }

      case Instruction::kLoad: {
        auto oldLoadInst = dynamic_cast<LoadInst *>(inst);
        auto oldPointer = oldLoadInst->getPointer();
        Value *newPointer;
        newPointer = oldNewValueMap.at(oldPointer);

        std::vector<Value *> newIndices;
        for (const auto &index : oldLoadInst->getIndices()) {
          newIndices.emplace_back(oldNewValueMap.at(index->getValue()));
        }
        ss << oldLoadInst->getName() << suffix;
        auto newLoadInst = new LoadInst(newPointer, newIndices, oldNewBlockMap.at(oldLoadInst->getParent()), ss.str());
        ss.str("");
        oldNewValueMap.emplace(oldLoadInst, newLoadInst);
        break;
      }

      case Instruction::kStore: {
        auto oldStoreInst = dynamic_cast<StoreInst *>(inst);
        auto oldPointer = oldStoreInst->getPointer();
        auto oldValue = oldStoreInst->getValue();
        Value *newPointer;
        Value *newValue;
        std::vector<Value *> newIndices;
        newPointer = oldNewValueMap.at(oldPointer);
        newValue = oldNewValueMap.at(oldValue);
        for (const auto &index : oldStoreInst->getIndices()) {
          newIndices.emplace_back(oldNewValueMap.at(index->getValue()));
        }
        auto newStoreInst = new StoreInst(newValue, newPointer, newIndices,
                                          oldNewBlockMap.at(oldStoreInst->getParent()), oldStoreInst->getName());
        oldNewValueMap.emplace(oldStoreInst, newStoreInst);
        break;
      }

      case Instruction::kLa: {
        auto oldLaInst = dynamic_cast<LaInst *>(inst);
        auto oldPointer = oldLaInst->getPointer();
        Value *newPointer;
        std::vector<Value *> newIndices;
        newPointer = oldNewValueMap.at(oldPointer);

        for (const auto &index : oldLaInst->getIndices()) {
          newIndices.emplace_back(oldNewValueMap.at(index->getValue()));
        }
        ss << oldLaInst->getName() << suffix;
        auto newLaInst = new LaInst(newPointer, newIndices, oldNewBlockMap.at(oldLaInst->getParent()), ss.str());
        ss.str("");
        oldNewValueMap.emplace(oldLaInst, newLaInst);
        break;
      }

      case Instruction::kGetSubArray: {
        auto oldGetSubArrayInst = dynamic_cast<GetSubArrayInst *>(inst);
        auto oldFather = oldGetSubArrayInst->getFatherArray();
        auto oldChild = oldGetSubArrayInst->getChildArray();
        Value *newFather;
        Value *newChild;
        std::vector<Value *> newIndices;
        newFather = oldNewValueMap.at(oldFather);
        newChild = oldNewValueMap.at(oldChild);

        for (const auto &index : oldGetSubArrayInst->getIndices()) {
          newIndices.emplace_back(oldNewValueMap.at(index->getValue()));
        }
        ss << oldGetSubArrayInst->getName() << suffix;
        auto newGetSubArrayInst =
            new GetSubArrayInst(dynamic_cast<LVal *>(newFather), dynamic_cast<LVal *>(newChild), newIndices,
                                oldNewBlockMap.at(oldGetSubArrayInst->getParent()), ss.str());
        ss.str("");
        oldNewValueMap.emplace(oldGetSubArrayInst, newGetSubArrayInst);
        break;
      }

      case Instruction::kMemset: {
        auto oldMemsetInst = dynamic_cast<MemsetInst *>(inst);
        auto oldPointer = oldMemsetInst->getPointer();
        auto oldValue = oldMemsetInst->getValue();
        Value *newPointer;
        Value *newValue;
        newPointer = oldNewValueMap.at(oldPointer);
        newValue = oldNewValueMap.at(oldValue);

        auto newMemsetInst = new MemsetInst(newPointer, oldMemsetInst->getBegin(), oldMemsetInst->getSize(), newValue,
                                            oldNewBlockMap.at(oldMemsetInst->getParent()), oldMemsetInst->getName());
        oldNewValueMap.emplace(oldMemsetInst, newMemsetInst);
        break;
      }

      case Instruction::kInvalid:
      case Instruction::kPhi: {
        break;
      }

      default:
        assert(false);
    }
    for (const auto &userUse : inst->getUses()) {
      auto user = userUse->getUser();
      if (!isAddedToCreate.at(user)) {
        toCreate.push(user);
        isAddedToCreate.at(user) = true;
      }
    }
  }

  for (const auto &oldBlock : blocks) {
    auto newBlock = oldNewBlockMap.at(oldBlock.get());
    builder.setPosition(newBlock, newBlock->end());
    for (const auto &inst : oldBlock->getInstructions()) {
      builder.insertInst(dynamic_cast<Instruction *>(oldNewValueMap.at(inst.get())));
    }
  }

  for (const auto &param : blocks.front()->getArguments()) {
    newFunction->getEntryBlock()->insertArgument(dynamic_cast<AllocaInst *>(oldNewValueMap.at(param)));
  }

  return newFunction;
}
/**
 * @brief 设置操作数
 *
 * @param [in] index 所要设置的操作数的位置
 * @param [in] value 所要设置成的value
 * @return 无返回值
 */
void User::setOperand(unsigned index, Value *value) {
  assert(index < getNumOperands());
  operands[index]->setValue(value);
  value->addUse(operands[index]);
}
/**
 * @brief 替换操作数
 *
 * @param [in] index 所要替换的操作数的位置
 * @param [in] value 所要替换成的value
 * @return 无返回值
 */
void User::replaceOperand(unsigned index, Value *value) {
  assert(index < getNumOperands());
  auto &use = operands[index];
  use->getValue()->removeUse(use);
  use->setValue(value);
  value->addUse(use);
}

CallInst::CallInst(Function *callee, const std::vector<Value *> &args, BasicBlock *parent, const std::string &name)
    : Instruction(kCall, callee->getReturnType(), parent, name) {
  addOperand(callee);
  for (auto arg : args) {
    addOperand(arg);
  }
}
/**
 * @brief 获取被调用函数的指针
 *
 * @return 被调用函数的指针
 */
auto CallInst::getCallee() -> Function * { return dynamic_cast<Function *>(getOperand(0)); }

/**
 * @brief 获取变量指针
 *
 * @param [in] name 变量名字
 * @return 变量指针
 */
auto SymbolTable::getVariable(const std::string &name) const -> User * {
  auto node = curNode;
  while (node != nullptr) {
    auto iter = node->varList.find(name);
    if (iter != node->varList.end()) {
      return iter->second;
    }
    node = node->pNode;
  }

  return nullptr;
}
/**
 * @brief 添加变量
 *
 * @param [in] name 变量名字
 * @param [in] variable 变量指针
 * @return 变量指针
 */
auto SymbolTable::addVariable(const std::string &name, User *variable) -> User * {
  User *result = nullptr;
  if (curNode != nullptr) {
    std::stringstream ss;
    auto iter = variableIndex.find(name);
    if (iter != variableIndex.end()) {
      ss << name << "(" << iter->second << ")";
      iter->second += 1;
    } else {
      variableIndex.emplace(name, 1);
      ss << name << "(" << 0 << ")";
    }

    variable->setName(ss.str());
    curNode->varList.emplace(name, variable);
    auto global = dynamic_cast<GlobalValue *>(variable);
    auto constvar = dynamic_cast<ConstantVariable *>(variable);
    if (global != nullptr) {
      globals.emplace_back(global);
    } else if (constvar != nullptr) {
      consts.emplace_back(constvar);
    }

    result = variable;
  }

  return result;
}
/**
 * @brief 获取全局变量
 *
 * @return 全局变量列表
 */
auto SymbolTable::getGlobals() -> std::vector<std::unique_ptr<GlobalValue>> & { return globals; }
/**
 * @brief 获取常量
 *
 * @return 常量列表
 */
auto SymbolTable::getConsts() const -> const std::vector<std::unique_ptr<ConstantVariable>> & { return consts; }
/**
 * @brief 进入新的作用域
 *
 * @return 无返回值
 */
void SymbolTable::enterNewScope() {
  auto newNode = new SymbolTableNode;
  nodeList.emplace_back(newNode);
  if (curNode != nullptr) {
    curNode->children.emplace_back(newNode);
  }
  newNode->pNode = curNode;
  curNode = newNode;
}
/**
 * @brief 进入全局作用域
 *
 * @return 无返回值
 */
void SymbolTable::enterGlobalScope() { curNode = nodeList.front().get(); }
/**
 * @brief 离开作用域
 *
 * @return 无返回值
 */
void SymbolTable::leaveScope() { curNode = curNode->pNode; }
/**
 * @brief 是否位于全局作用域
 *
 * @return 布尔值
 */
auto SymbolTable::isInGlobalScope() const -> bool { return curNode->pNode == nullptr; }

/**
 * @brief  判断是否为循环不变量
 * @param  value: 要判断的value
 * @return true: 是不变量
 * @return false: 不是
 */
auto Loop::isSimpleLoopInvariant(Value *value) -> bool {
  // auto constValue = dynamic_cast<ConstantValue *>(value);
  // if (constValue != nullptr) {
  //   return false;
  // }
  if (auto instr = dynamic_cast<Instruction *>(value)) {
    if (instr->isLoad()) {
      auto loadinst = dynamic_cast<LoadInst *>(instr);

      auto loadvalue = dynamic_cast<AllocaInst *>(loadinst->getOperand(0));
      if (loadvalue != nullptr) {
        if (loadvalue->getParent() != nullptr) {
          auto basicblock = loadvalue->getParent();
          return !this->isBasicBlockInLoop(basicblock);
        }
      }
      auto globalvalue = dynamic_cast<GlobalValue *>(loadinst->getOperand(0));
      if (globalvalue != nullptr) {
        return true;
      }
      auto basicblock = instr->getParent();

      return !this->isBasicBlockInLoop(basicblock);
    }
    auto basicblock = instr->getParent();
    return !this->isBasicBlockInLoop(basicblock);
  }
  return true;
}

/**
 * @brief 移动指令
 *
 * @param [in] sourcePos 源指令列表位置
 * @param [in] targetPos 目的指令列表位置
 * @param [in] block 目标基本块
 * @return 无返回值
 */
auto BasicBlock::moveInst(iterator sourcePos, iterator targetPos, BasicBlock *block) -> iterator {
  auto inst = sourcePos->release();
  inst->setParent(block);
  block->instructions.emplace(targetPos, inst);
  return instructions.erase(sourcePos);
}

}  // namespace sysy
