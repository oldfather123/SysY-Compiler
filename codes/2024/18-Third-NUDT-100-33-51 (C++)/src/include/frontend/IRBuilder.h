#pragma once

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../frontend/IR.h"

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
  auto getLabelIndex() -> unsigned {
    labelIndex += 1;
    return labelIndex - 1;
  }  ///< 获取基本块标签编号
  auto getTmpIndex() -> unsigned {
    tmpIndex += 1;
    return tmpIndex - 1;
  }                                                                          ///< 获取临时变量编号
  auto getBasicBlock() const -> BasicBlock * { return block; }               ///< 获取当前基本块
  auto getBreakBlock() const -> BasicBlock * { return breakBlocks.back(); }  ///< 获取break目标块
  auto popBreakBlock() -> BasicBlock * {
    auto result = breakBlocks.back();
    breakBlocks.pop_back();
    return result;
  }                                                                                ///< 弹出break目标块
  auto getContinueBlock() const -> BasicBlock * { return continueBlocks.back(); }  ///< 获取continue目标块
  auto popContinueBlock() -> BasicBlock * {
    auto result = continueBlocks.back();
    continueBlocks.pop_back();
    return result;
  }  ///< 弹出continue目标块

  auto getTrueBlock() const -> BasicBlock * { return trueBlocks.back(); }    ///< 获取true分支基本块
  auto getFalseBlock() const -> BasicBlock * { return falseBlocks.back(); }  ///< 获取false分支基本块
  auto popTrueBlock() -> BasicBlock * {
    auto result = trueBlocks.back();
    trueBlocks.pop_back();
    return result;
  }  ///< 弹出true分支基本块
  auto popFalseBlock() -> BasicBlock * {
    auto result = falseBlocks.back();
    falseBlocks.pop_back();
    return result;
  }                                                                      ///< 弹出false分支基本块
  auto getPosition() const -> BasicBlock::iterator { return position; }  ///< 获取当前基本块指令列表位置的迭代器
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
  auto insertInst(Instruction *inst) -> Instruction * {
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 插入指令
  auto createUnaryInst(Instruction::Kind kind, Type *type, Value *operand, const std::string &name = "")
      -> UnaryInst * {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << "%" << tmpIndex;
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
  auto createNegInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kNeg, Type::getIntType(), operand, name);
  }  ///< 创建取反指令
  auto createNotInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kNot, Type::getIntType(), operand, name);
  }  ///< 创建取非指令
  auto createFtoIInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kFtoI, Type::getIntType(), operand, name);
  }  ///< 创建浮点转整型指令
  auto createBitFtoIInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kBitFtoI, Type::getIntType(), operand, name);
  }  ///< 创建按位浮点转整型指令
  auto createFNegInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kFNeg, Type::getFloatType(), operand, name);
  }  ///< 创建浮点取反指令
  auto createFNotInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kFNot, Type::getIntType(), operand, name);
  }  ///< 创建浮点取非指令
  auto createIToFInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kItoF, Type::getFloatType(), operand, name);
  }  ///< 创建整型转浮点指令
  auto createBitItoFInst(Value *operand, const std::string &name = "") -> UnaryInst * {
    return createUnaryInst(Instruction::kBitItoF, Type::getFloatType(), operand, name);
  }  ///< 创建按位整型转浮点指令
  auto createBinaryInst(Instruction::Kind kind, Type *type, Value *lhs, Value *rhs, const std::string &name = "")
      -> BinaryInst * {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << "%" << tmpIndex;
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
  auto createAddInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kAdd, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建加法指令
  auto createSubInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kSub, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建减法指令
  auto createMulInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kMul, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建乘法指令
  auto createDivInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kDiv, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建除法指令
  auto createRemInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kRem, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建取余指令
  auto createICmpEQInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpEQ, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建相等设置指令
  auto createICmpNEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpNE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建不相等设置指令
  auto createICmpLTInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpLT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建小于设置指令
  auto createICmpLEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpLE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建小于等于设置指令
  auto createICmpGTInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpGT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建大于设置指令
  auto createICmpGEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kICmpGE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建大于等于设置指令
  auto createFAddInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFAdd, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点加法指令
  auto createFSubInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFSub, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点减法指令
  auto createFMulInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFMul, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点乘法指令
  auto createFDivInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFDiv, Type::getFloatType(), lhs, rhs, name);
  }  ///< 创建浮点除法指令
  auto createFCmpEQInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpEQ, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点相等设置指令
  auto createFCmpNEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpNE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点不相等设置指令
  auto createFCmpLTInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpLT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点小于设置指令
  auto createFCmpLEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpLE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点小于等于设置指令
  auto createFCmpGTInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpGT, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点大于设置指令
  auto createFCmpGEInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kFCmpGE, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建浮点相大于等于设置指令
  auto createAndInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kAnd, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建按位且指令
  auto createOrInst(Value *lhs, Value *rhs, const std::string &name = "") -> BinaryInst * {
    return createBinaryInst(Instruction::kOr, Type::getIntType(), lhs, rhs, name);
  }  ///< 创建按位或指令
  auto createCallInst(Function *callee, const std::vector<Value *> &args, const std::string &name = "") -> CallInst * {
    std::string newName;
    if (name.empty() && callee->getReturnType() != Type::getVoidType()) {
      std::stringstream ss;
      ss << "%" << tmpIndex;
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
  auto createReturnInst(Value *value = nullptr, const std::string &name = "") -> ReturnInst * {
    auto inst = new ReturnInst(value, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建return指令
  auto createUncondBrInst(BasicBlock *thenBlock, const std::vector<Value *> &args) -> UncondBrInst * {
    auto inst = new UncondBrInst(thenBlock, args, block);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建无条件指令
  auto createCondBrInst(Value *condition, BasicBlock *thenBlock, BasicBlock *elseBlock,
                        const std::vector<Value *> &thenArgs, const std::vector<Value *> &elseArgs) -> CondBrInst * {
    auto inst = new CondBrInst(condition, thenBlock, elseBlock, thenArgs, elseArgs, block);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建条件跳转指令
  auto createAllocaInst(Type *type, const std::vector<Value *> &dims = {}, const std::string &name = "")
      -> AllocaInst * {
    auto inst = new AllocaInst(type, dims, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建分配指令
  auto createAllocaInstWithoutInsert(Type *type, const std::vector<Value *> &dims = {}, BasicBlock *parent = nullptr,
                                     const std::string &name = "") -> AllocaInst * {
    auto inst = new AllocaInst(type, dims, parent, name);
    assert(inst);
    return inst;
  }  ///< 创建不插入指令列表的分配指令
  auto createLoadInst(Value *pointer, const std::vector<Value *> &indices = {}, const std::string &name = "")
      -> LoadInst * {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << "%" << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new LoadInst(pointer, indices, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建load指令
  auto createLaInst(Value *pointer, const std::vector<Value *> &indices = {}, const std::string &name = "")
      -> LaInst * {
    std::string newName;
    if (name.empty()) {
      std::stringstream ss;
      ss << "%" << tmpIndex;
      newName = ss.str();
      tmpIndex++;
    } else {
      newName = name;
    }

    auto inst = new LaInst(pointer, indices, block, newName);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建la指令
  auto createGetSubArray(LVal *fatherArray, const std::vector<Value *> &indices, const std::string &name = "")
      -> GetSubArrayInst * {
    assert(fatherArray->getLValNumDims() > indices.size());
    std::vector<Value *> subDims;
    auto dims = fatherArray->getLValDims();
    auto iter = std::next(dims.begin(), indices.size());
    while (iter != dims.end()) {
      subDims.emplace_back(*iter);
      iter++;
    }

    std::string childArrayName;
    std::stringstream ss;
    ss << "A"
       << "%" << tmpIndex;
    childArrayName = ss.str();
    tmpIndex++;

    auto fatherArrayValue = dynamic_cast<Value *>(fatherArray);
    auto childArray = new AllocaInst(fatherArrayValue->getType(), subDims, block, childArrayName);
    auto inst = new GetSubArrayInst(fatherArray, childArray, indices, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建获取部分数组指令
  auto createMemsetInst(Value *pointer, Value *begin, Value *size, Value *value, const std::string &name = "")
      -> MemsetInst * {
    auto inst = new MemsetInst(pointer, begin, size, value, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建memset指令
  auto createStoreInst(Value *value, Value *pointer, const std::vector<Value *> &indices = {},
                       const std::string &name = "") -> StoreInst * {
    auto inst = new StoreInst(value, pointer, indices, block, name);
    assert(inst);
    block->getInstructions().emplace(position, inst);
    return inst;
  }  ///< 创建store指令
  auto createPhiInst(Type *type, Value *lhs, BasicBlock *parent, const std::string &name = "") -> PhiInst * {
    auto predNum = parent->getNumPredecessors();
    std::vector<Value *> rhs;
    for (size_t i = 0; i < predNum; i++) {
      rhs.push_back(lhs);
    }
    auto inst = new PhiInst(type, lhs, rhs, lhs, parent, name);
    assert(inst);
    parent->getInstructions().emplace(parent->begin(), inst);
    return inst;
  }  ///< 创建Phi指令
};

}  // namespace sysy
