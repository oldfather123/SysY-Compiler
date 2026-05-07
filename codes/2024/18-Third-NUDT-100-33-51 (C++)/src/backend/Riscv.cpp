#include "../include/backend/Riscv.h"
#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <utility>

/**
 * @file RISCV.cpp
 *
 * @brief 定义RISC-V汇编相关的类型与操作的源文件
 */

namespace riscv {
auto IntPhyRegister::getIntPhyRegister(IntRegisterKind reg) -> IntPhyRegister * {
  static std::map<IntRegisterKind, IntPhyRegister> regs;

  auto iter = regs.find(reg);
  if (iter != regs.end()) {
    return &iter->second;
  }
  regs.emplace(reg, IntPhyRegister(reg));

  return &(regs.find(reg)->second);
}

auto FloatPhyRegister::getFloatPhyRegister(FloatRegisterKind reg) -> FloatPhyRegister * {
  static std::map<FloatRegisterKind, FloatPhyRegister> regs;

  auto iter = regs.find(reg);
  if (iter != regs.end()) {
    return &iter->second;
  }
  regs.emplace(reg, FloatPhyRegister(reg));

  return &(regs.find(reg)->second);
}

auto Function::addBasicBlock(const std::string &name) -> BasicBlock * {
  auto block = new BasicBlock(this, name);
  blocks.emplace_back(block);
  return block;
}

auto Module::createFunction(const std::string &name, Function::ReturnType returnType, bool isExternal) -> Function * {
  auto function = new Function(this, name, returnType, isExternal);
  assert(function);
  functions.emplace(name, function);
  return function;
}

auto Module::createGlobal(bool isInt, const std::vector<unsigned> &dims, const ConstantCounter &init,
                          const std::string &name) -> Global * {
  auto global = new Global(isInt, dims, init, name);
  assert(global);
  globals.emplace(name, global);
  return global;
}

auto Module::getFunction(const std::string &name) const -> Function * {
  auto iter = functions.find(name);
  assert(iter != functions.end());
  return iter->second.get();
}

auto Module::getGlobal(const std::string &name) const -> Global * {
  auto iter = globals.find(name);
  assert(iter != globals.end());
  return iter->second.get();
}

void Module::renameGlobals() {
  uint64_t globalIndex = 0;
  for (const auto &globalItem : globals) {
    std::stringstream ss;
    auto global = globalItem.second.get();
    if (global->getName()[0] != '.') {
      ss << ".G" << globalIndex;
      global->setName(ss.str());
      ss.str("");
      globalIndex++;
    }
  }
}

/**
 * @brief 按照执行的正确顺序排序基本块
 *
 * 采用了深度优先排序，注意到对于条件跳转指令结尾的基本块，优先选择elseBlock排列在其后
 * 必须保证elseBlock有唯一前驱
 */
void Function::sortBlocks() {
  std::map<BasicBlock *, unsigned> blockOrder;
  std::map<BasicBlock *, bool> visited;
  std::stack<BasicBlock *> visitStack;
  for (const auto &block : blocks) {
    visited.emplace(block.get(), false);
  }

  auto order = 0;
  visitStack.push(blocks.front().get());
  while (!visitStack.empty()) {
    auto curBlock = visitStack.top();
    visitStack.pop();
    visited[curBlock] = true;
    blockOrder.emplace(curBlock, order);

    auto successors = curBlock->getSuccessors();
    if (curBlock->back()->isBInst()) {
      auto bInst = dynamic_cast<BInst *>(curBlock->back().get());
      auto thenBlock = bInst->getThenBlock();
      if (!visited[thenBlock]) {
        visitStack.push(thenBlock);
      }
      for (const auto &succ : successors) {
        if (succ != thenBlock && !visited[succ]) {
          visitStack.push(succ);
        }
      }
    } else {
      for (const auto &succ : successors) {
        if (!visited[succ]) {
          visitStack.push(succ);
        }
      }
    }
    order++;
  }

  auto cmp = [&](const std::unique_ptr<BasicBlock> &lhs, const std::unique_ptr<BasicBlock> &rhs) -> bool {
    return blockOrder[lhs.get()] < blockOrder[rhs.get()];
  };
  blocks.sort(cmp);
}

/**
 * @brief 重命名基本块
 *
 * 配合块排序使用，方便阅读汇编
 */
auto Function::renameBlocks(uint64_t begin) -> uint64_t {
  std::set<BasicBlock *> targetBlocks;
  for (const auto &block : blocks) {
    auto inst = block->back().get();
    auto bInst = dynamic_cast<BInst *>(inst);
    auto jInst = dynamic_cast<JInst *>(inst);
    if (bInst != nullptr) {
      targetBlocks.insert(bInst->getThenBlock());
    } else if (jInst != nullptr) {
      targetBlocks.insert(dynamic_cast<BasicBlock *>(jInst->getLabel()));
    }
    targetBlocks.erase(nullptr);
  }

  uint64_t index = begin;
  std::stringstream ss;
  for (const auto &block : blocks) {
    if (targetBlocks.find(block.get()) != targetBlocks.end()) {
      ss << ".LC" << index;
      block->setName(ss.str());
      ss.str("");
      index++;
    }
  }
  return index;
}

void BasicBlock::mergeBlock(BasicBlock *block) {
  for (auto &inst : block->getInstructions()) {
    inst->setParent(this);
    instructions.emplace_back(inst.release());
  }
  block->getParent()->removeBlock(block);
}
}  // namespace riscv
