#pragma once

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "IR.h"

/**
 * @file IRBuilder.h
 *
 * @brief 定义IR构建器的头文件
 */
namespace sysy {

/**
 * @brief 中间IR的构建器
 *
 */
class IRBuilder {
 private:
  unsigned labelIndex;  ///< 基本块标签编号
  unsigned tmpIndex;    ///< 临时变量编号

  BasicBlock *block;              ///< 当前基本块
  BasicBlock::iterator position;  ///< 当前基本块指令列表位置的迭代器

  std::vector<BasicBlock *> trueBlocks;   ///< true分支基本块列表
  std::vector<BasicBlock *> falseBlocks;  ///< false分支基本块列表

  std::vector<BasicBlock *> breakBlocks;     ///< break目标块列表
  std::vector<BasicBlock *> continueBlocks;  ///< continue目标块列表

 public:
  IRBuilder() : labelIndex(0), tmpIndex(0), block(nullptr) {}
  explicit IRBuilder(BasicBlock *block) : labelIndex(0), tmpIndex(0), block(block), position(block->end()) {}
  IRBuilder(BasicBlock *block, BasicBlock::iterator position)
      : labelIndex(0), tmpIndex(0), block(block), position(position) {}

 public:
  unsigned getLabelIndex() {
    labelIndex += 1;
    return labelIndex - 1;
  }  ///< 获取基本块标签编号
  unsigned getTmpIndex() {
    tmpIndex += 1;
    return tmpIndex - 1;
  }                                                                          ///< 获取临时变量编号
  BasicBlock * getBasicBlock() const { return block; }               ///< 获取当前基本块
  BasicBlock * getBreakBlock() const { return breakBlocks.back(); }  ///< 获取break目标块
  BasicBlock * popBreakBlock() {
    auto result = breakBlocks.back();
    breakBlocks.pop_back();
    return result;
  }                                                                                ///< 弹出break目标块
  BasicBlock * getContinueBlock() const { return continueBlocks.back(); }  ///< 获取continue目标块
  BasicBlock * popContinueBlock() {
    auto result = continueBlocks.back();
    continueBlocks.pop_back();
    return result;
  }  ///< 弹出continue目标块

  BasicBlock * getTrueBlock() const { return trueBlocks.back(); }    ///< 获取true分支基本块
  BasicBlock * getFalseBlock() const { return falseBlocks.back(); }  ///< 获取false分支基本块
  BasicBlock * popTrueBlock() {
    auto result = trueBlocks.back();
    trueBlocks.pop_back();
    return result;
  }  ///< 弹出true分支基本块
  BasicBlock * popFalseBlock() {
    auto result = falseBlocks.back();
    falseBlocks.pop_back();
    return result;
  }                                                                      ///< 弹出false分支基本块
  BasicBlock::iterator getPosition() const { return position; }  ///< 获取当前基本块指令列表位置的迭代器
  void setPosition(BasicBlock *block, BasicBlock::iterator position) {
    this->block = block;
    this->position = position;
  }  ///< 设置基本块和基本块指令列表位置的迭代器
  void setPosition(BasicBlock::iterator position) {
    this->position = position;
  }  ///< 设置当前基本块指令列表位置的迭代器
  void pushBreakBlock(BasicBlock *block) { breakBlocks.push_back(block); }        ///< 压入break目标基本块
  void pushContinueBlock(BasicBlock *block) { continueBlocks.push_back(block); }  ///< 压入continue目标基本块
  void pushTrueBlock(BasicBlock *block) { trueBlocks.push_back(block); }          ///< 压入true分支基本块
  void pushFalseBlock(BasicBlock *block) { falseBlocks.push_back(block); }        ///< 压入false分支基本块

 public:
  Instruction * insertInst(Instruction *inst) {
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 插入指令
  UnaryInst * createUnaryInst(Instruction::Kind kind, Type *type, Value *operand, const std::string &name = "") {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new UnaryInst(kind, type, operand, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建一元指令
  UnaryInst * createNegInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kNeg, Type::getIntType(), operand, name);
  }  ///< 创建取反指令
  UnaryInst * createNotInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kNot, Type::getIntType(), operand, name);
  }  ///< 创建取非指令
  UnaryInst * createFtoIInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kFtoI, Type::getIntType(), operand, name);
  }  ///< 创建浮点转整型指令
  UnaryInst * createBitFtoIInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kBitFtoI, Type::getIntType(), operand, name);
  }  ///< 创建按位浮点转整型指令
  UnaryInst * createFNegInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kFNeg, Type::getFloatType(), operand, name);
  }  ///< 创建浮点取反指令
  UnaryInst * createFNotInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kFNot, Type::getIntType(), operand, name);
  }  ///< 创建浮点取非指令
  UnaryInst * createItoFInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kItoF, Type::getFloatType(), operand, name);
  }  ///< 创建整型转浮点指令
  UnaryInst * createBitItoFInst(Value *operand, const std::string &name = "") {
    return createUnaryInst(Instruction::kBitItoF, Type::getFloatType(), operand, name);
  }  ///< 创建按位整型转浮点指令
  BinaryInst * createBinaryInst(Instruction::Kind kind, Type *type, Value *lhs, Value *rhs, const std::string &name = "") {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new BinaryInst(kind, type, lhs, rhs, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建二元指令
  BinaryInst * createAddInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kAdd, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建加法指令
  BinaryInst * createSubInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kSub, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建减法指令
  BinaryInst * createMulInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kMul, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建乘法指令
  BinaryInst * createDivInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kDiv, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建除法指令
  BinaryInst * createRemInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kRem, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建取余指令
  BinaryInst * createICmpEQInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpEQ, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建相等设置指令
  BinaryInst * createICmpNEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpNE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建不相等设置指令
  BinaryInst * createICmpLTInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpLT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建小于设置指令
  BinaryInst * createICmpLEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpLE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建小于等于设置指令
  BinaryInst * createICmpGTInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpGT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建大于设置指令
  BinaryInst * createICmpGEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kICmpGE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建大于等于设置指令
  BinaryInst * createFAddInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFAdd, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点加法指令
  BinaryInst * createFSubInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFSub, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点减法指令
  BinaryInst * createFMulInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFMul, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点乘法指令
  BinaryInst * createFDivInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFDiv, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点除法指令
  BinaryInst * createFCmpEQInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpEQ, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点相等设置指令
  BinaryInst * createFCmpNEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpNE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点不相等设置指令
  BinaryInst * createFCmpLTInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpLT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点小于设置指令
  BinaryInst * createFCmpLEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpLE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点小于等于设置指令
  BinaryInst * createFCmpGTInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpGT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点大于设置指令
  BinaryInst * createFCmpGEInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kFCmpGE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点相大于等于设置指令
  BinaryInst * createAndInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kAnd, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建按位且指令
  BinaryInst * createOrInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kOr, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建按位或指令
  BinaryInst * createSllInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kSll, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建逻辑左移指令
  BinaryInst * createSrlInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kSrl, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建逻辑右移指令
  BinaryInst * createSraInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kSra, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建算术右移指令
  BinaryInst * createMulhInst(Value *lhs, Value *rhs, const std::string &name = "") {
    return createBinaryInst(Instruction::kMulh, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建高位乘法指令
  CallInst * createCallInst(Function *callee, const std::vector<Value *> &args, const std::string &name = "") {
    std::string newName;
    if (name.empty() && callee->getReturnType() != Type::getVoidType()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new CallInst(callee, args, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建Call指令
  ReturnInst * createReturnInst(Value *value = nullptr, const std::string &name = "") {
    auto inst = new ReturnInst(value, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建return指令
  UncondBrInst * createUncondBrInst(BasicBlock *thenBlock) {
    auto inst = new UncondBrInst(thenBlock, block);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建无条件指令
  CondBrInst * createCondBrInst(Value *condition, BasicBlock *thenBlock, BasicBlock *elseBlock) {
    auto inst = new CondBrInst(condition, thenBlock, elseBlock, block);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建条件跳转指令
  UnreachableInst * createUnreachableInst(const std::string &name = "") {
    auto inst = new UnreachableInst(name, block);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建不可达指令
  AllocaInst * createAllocaInst(Type *type, const std::string &name = "") {
    auto inst = new AllocaInst(type, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建分配指令
  LoadInst * createLoadInst(Value *pointer, const std::vector<Value *> &indices = {}, const std::string &name = "") {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new LoadInst(pointer, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建load指令
  MemsetInst * createMemsetInst(Value *pointer, Value *begin, Value *size, Value *value, const std::string &name = "") {
    auto inst = new MemsetInst(pointer, begin, size, value, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建memset指令
  StoreInst * createStoreInst(Value *value, Value *pointer, const std::string &name = "") {
    auto inst = new StoreInst(value, pointer, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建store指令
  PhiInst * createPhiInst(Type *type, const std::vector<Value*> &vals = {}, const std::vector<BasicBlock*> &blks = {}, const std::string &name = "") {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }
    auto inst = new PhiInst(type, vals, blks, block, newName);
    assert(inst);
    block->getInstructions().emplace(block->begin(), inst);
    return inst;
  }  ///< 创建Phi指令
  /**
     * @brief 根据 LLVM 设计模式创建 GEP 指令。
     * 它会自动推断返回类型，无需手动指定。
     */
  GetElementPtrInst *createGetElementPtrInst(Value *basePointer, const std::vector<Value *> &indices,
                                             const std::string &name = "") {
    Type *ResultElementType = getIndexedType(basePointer->getType(), indices);
    if (!ResultElementType) {
      assert(false && "Invalid GEP indexing!");
      return nullptr;
    }
    Type *ResultType = PointerType::get(ResultElementType);
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new GetElementPtrInst(ResultType, basePointer, indices, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }

  static Type *getIndexedType(Type *pointerType, const std::vector<Value *> &indices) {
    assert(pointerType->isPointer() && "base must be a pointer type!");
    // GEP 的类型推断从基指针所指向的类型开始。
    // 例如：
    // - 如果 pointerType 是 `[20 x [10 x i32]]*`，`currentWalkType` 初始为 `[20 x [10 x i32]]`。
    // - 如果 pointerType 是 `i32*`，`currentWalkType` 初始为 `i32`。
    // - 如果 pointerType 是 `i32**`，`currentWalkType` 初始为 `i32*`。
    Type *currentWalkType = pointerType->as<PointerType>()->getBaseType();

    // 遍历所有索引来深入类型层次结构。
    // 重要：第一个索引总是用于"解引用"指针，后续索引才用于数组/结构体的索引
    for (int i = 0; i < indices.size(); ++i) {
        if (i == 0) {
            // 第一个索引：总是用于"解引用"基指针，不改变currentWalkType
            // 例如：对于 `[4 x i32]* ptr, i32 0`，第一个0只是说"访问ptr指向的对象"
            // currentWalkType 保持为 `[4 x i32]`
            continue;
        } else {
            // 后续索引：用于实际的数组/结构体索引
            if (currentWalkType->isArray()) {
                // 数组索引：选择数组中的元素
                currentWalkType = currentWalkType->as<ArrayType>()->getElementType();
            } else if (currentWalkType->isPointer()) {
                // 指针索引：解引用指针并继续
                currentWalkType = currentWalkType->as<PointerType>()->getBaseType();
            } else {
                // 标量类型：不能进一步索引
                if (i < indices.size() - 1) { 
                    assert(false && "Invalid GEP indexing: attempting to index into a non-aggregate/non-pointer type with further indices.");
                    return nullptr;
                }
            }
        }
    }
    
    // 所有索引处理完毕后，`currentWalkType` 就是 GEP 指令最终计算出的地址所指向的元素的类型。
    return currentWalkType;
  }
};

}  // namespace sysy
