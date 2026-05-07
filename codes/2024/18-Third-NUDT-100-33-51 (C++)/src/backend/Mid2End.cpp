#include "../include/backend/Mid2End.h"
#include <sys/types.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <vector>
#include "../include/backend/Riscv.h"
#include "../include/backend/RiscvBuilder.h"
#include "../include/frontend/IR.h"
#include "../include/utils/range.h"

/**
 * @file Mid2End.cpp
 *
 * @brief 定义中端IR转化为后端IR的转化器的源文件
 */

namespace mid2end {
const std::map<riscv::PhyRegister *, bool> StackTable::isCalleeReserve = {
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t0), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t1), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t2), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s1), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a3), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a4), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a5), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a6), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a7), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s2), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s3), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s4), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s5), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s6), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s7), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s8), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s9), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s10), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::s11), true},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t3), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t4), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t5), false},
    {riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t6), false},

    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft0), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft1), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft2), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft3), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft4), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft5), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft6), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft7), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs0), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs1), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa1), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa2), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa3), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa4), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa5), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa6), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa7), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs2), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs3), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs4), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs5), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs6), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs7), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs8), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs9), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs10), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fs11), true},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft8), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft9), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft10), false},
    {riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft11), false}};
void StackTable::addSpillParam(riscv::SymRegister *param) {
  assert(!isInsertEnd);
  assert(spillParams.find(param) == spillParams.end());
  auto intParam = dynamic_cast<riscv::IntSymRegister *>(param);
  if (intParam != nullptr) {
    if (intParam->is64Value()) {
      if (outStackSize % 8 != 0) {
        outStackSize += (8 - outStackSize % 8);
      }
      spillParams.emplace(param, outStackSize);
      outStackSize += 8;
    } else {
      if (outStackSize % 4 != 0) {
        outStackSize += (4 - outStackSize % 4);
      }
      spillParams.emplace(param, outStackSize);
      outStackSize += 4;
    }
  } else {
    if (outStackSize % 4 != 0) {
      outStackSize += (4 - outStackSize % 4);
    }
    spillParams.emplace(param, outStackSize);
    outStackSize += 4;
  }
}
void StackTable::addNotSpillParamReg(riscv::PhyRegister *reg) {
  assert(!isInsertEnd);
  notSpillParamRegs.insert(reg);
}
void StackTable::addSpill(riscv::SymRegister *spill) {
  assert(!isInsertEnd);
  assert(spills.find(spill) == spills.end());
  auto intSpill = dynamic_cast<riscv::IntSymRegister *>(spill);
  if (intSpill != nullptr) {
    if (intSpill->is64Value()) {
      stackSize += 8;
      if (stackSize % 8 != 0) {
        stackSize += (8 - stackSize % 8);
      }
      spills.emplace(spill, stackSize);
    } else {
      stackSize += 4;
      if (stackSize % 4 != 0) {
        stackSize += (4 - stackSize % 4);
      }
      spills.emplace(spill, stackSize);
    }
  } else {
    stackSize += 4;
    if (stackSize % 4 != 0) {
      stackSize += (4 - stackSize % 4);
    }
    spills.emplace(spill, stackSize);
  }
}
void StackTable::addLocalArray(LocalArray *localArray) {
  assert(!isInsertEnd);
  assert(localArrays.find(localArray) == localArrays.end());
  unsigned size = 1;
  for (const auto &dim : localArray->getDims()) {
    size *= dim;
  }
  stackSize += 4 * size;
  if (stackSize % 4 != 0) {
    stackSize += (4 - stackSize % 4);
  }
  localArrays.emplace(localArray, stackSize);
}
// 浮点可以只存4个字节？
void StackTable::addPhyReg(riscv::PhyRegister *reg) {
  assert(!isInsertEnd);
  if (isCalleeReserve.at(reg)) {
    if (calleeReserveRegisterMap.find(reg) == calleeReserveRegisterMap.end()) {
      stackSize += 8;
      if (stackSize % 8 != 0) {
        stackSize += (8 - stackSize % 8);
      }
      calleeReserveRegisterMap.emplace(reg, stackSize);
    }
  } else {
    callerReserveRegisters.insert(reg);
  }
}
void StackTable::addActiveAtCallInst(riscv::CallInst *callInst, riscv::PhyRegister *reg) {
  assert(!isInsertEnd);
  activeAtCallInst.at(callInst).insert(reg);
}
void StackTable::addCalleeStackTable(riscv::CallInst *callInst, StackTable *callee) {
  callees.emplace(callInst, callee);
}
auto StackTable::getSpillParamAddr(riscv::SymRegister *param) const -> uint64_t {
  auto iter = spillParams.find(param);
  assert(iter != spillParams.end());
  return iter->second;
}
auto StackTable::getLocalArrayAddr(LocalArray *localArray) const -> uint64_t {
  auto iter = localArrays.find(localArray);
  assert(iter != localArrays.end());
  return iter->second;
}
auto StackTable::getSpillAddr(riscv::SymRegister *spill) const -> uint64_t {
  auto iter = spills.find(spill);
  assert(iter != spills.end());
  return iter->second;
}
void StackTable::endSpillInsert() {
  assert(!isInsertEnd);
  isInsertEnd = true;

  auto maxTmp = stackSize;
  for (const auto &activeItem : activeAtCallInst) {
    auto callInst = activeItem.first;
    auto calleeStackTable = callees.at(callInst);
    callerReserveRegisterMap.emplace(callInst, std::map<riscv::PhyRegister *, uint64_t>{});
    std::set<riscv::PhyRegister *> modifiedRegs;
    auto notSpillRegs = calleeStackTable->getNotSpillParamRegs();
    auto callerReserveRegs = calleeStackTable->getCallerReserveRegs();
    modifiedRegs.merge(notSpillRegs);
    modifiedRegs.merge(callerReserveRegs);

    std::set<riscv::PhyRegister *> reserveRegs;
    std::set_intersection(activeItem.second.begin(), activeItem.second.end(), modifiedRegs.begin(), modifiedRegs.end(),
                          std::inserter(reserveRegs, reserveRegs.begin()));
    auto outStackTmp = calleeStackTable->getOutStackSize();
    if (callerReserveRegisterMap.find(callInst) == callerReserveRegisterMap.end()) {
      callerReserveRegisterMap.emplace(callInst, std::map<riscv::PhyRegister *, uint64_t>{});
    }
    for (const auto &reg : reserveRegs) {
      if (outStackTmp % 8 != 0) {
        outStackTmp += (8 - outStackTmp % 8);
      }
      callerReserveRegisterMap.at(callInst).emplace(reg, outStackTmp);
      outStackTmp += 8;
    }

    auto tmp = stackSize + outStackTmp;
    if (tmp % 8 != 0) {
      tmp += (8 - tmp % 8);
    }
    if (tmp > maxTmp) {
      maxTmp = tmp;
    }
  }
  stackSize = maxTmp;
}
auto StackTable::getStackSize() const -> uint64_t {
  assert(isInsertEnd);
  return stackSize;
}
std::list<riscv::IntPhyRegister *> CodeGenerater::intTmpUseRegs = {
    riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t0),
    riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::t1)};
std::list<riscv::FloatPhyRegister *> CodeGenerater::floatTmpUseRegs = {
    riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft0),
    riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft1),
    riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::ft2)};
auto CodeGenerater::getPhyReg(unsigned position, riscv::SymRegister *symbol) -> riscv::PhyRegister * {
  if (allocator.getAllocaResult().find(symbol) == allocator.getAllocaResult().end() ||
      allocator.isSymbolSpilled(symbol)) {
    return nullptr;
  }
  for (const auto &interval : allocator.getAllocaResult().at(symbol)) {
    if (interval->left <= position && interval->right >= position) {
      return interval->physicRegister;
    }
  }
  return nullptr;
}
void CodeGenerater::initStackTable() {
  auto pModule = pSelector->getModule();
  // 不动点法求需要保存的寄存器
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    auto stackTable = pSelector->getStackTable(function);
    if (!function->isExternalFucntion()) {
      for (const auto &symbol : allocator.getFunctionSymbolMap().at(function)) {
        if (!allocator.isSymbolSpilled(symbol)) {
          for (const auto &interval : allocator.getAllocaResult().at(symbol)) {
            stackTable->addPhyReg(interval->physicRegister);
          }
        }
      }
      for (const auto &calleeItem : stackTable->getCallees()) {
        auto calleeStackTable = calleeItem.second;
        for (const auto &reg : calleeStackTable->getNotSpillParamRegs()) {
          stackTable->addPhyReg(reg);
        }
      }
      stackTable->addPhyReg(riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra));
      if (function->getReturnType() == riscv::Function::INT) {
        stackTable->addPhyReg(riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0));
      } else if (function->getReturnType() == riscv::Function::FLOAT) {
        stackTable->addPhyReg(riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0));
      }
    } else {
      // 保守保存所有可能的寄存器
      for (const auto &regItem : mid2end::StackTable::isCalleeReserve) {
        if (!regItem.second) {
          stackTable->addPhyReg(regItem.first);
        }
      }
    }
  }
  bool changed;
  do {
    changed = false;
    for (const auto &functionItem : pModule->getFunctions()) {
      auto function = functionItem.second.get();
      if (!function->isExternalFucntion()) {
        auto stackTable = pSelector->getStackTable(function);
        auto callerReserveRegs = stackTable->getCallerReserveRegs();
        for (const auto &calleeItem : stackTable->getCallees()) {
          for (const auto &reg : calleeItem.second->getCallerReserveRegs()) {
            if (callerReserveRegs.find(reg) == callerReserveRegs.end()) {
              stackTable->addPhyReg(reg);
              changed = true;
            }
          }
        }
      }
    }
  } while (changed);

  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto stackTable = pSelector->getStackTable(function);
      for (const auto &symbol : allocator.getFunctionSymbolMap().at(function)) {
        if (allocator.isSymbolSpilled(symbol)) {
          stackTable->addSpill(symbol);
        }
      }
    }
  }
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto stackTable = pSelector->getStackTable(function);
      for (const auto &block : function->getBlocks()) {
        auto pos = allocator.getBlockList().at(block.get());
        for (const auto &inst : block->getInstructions()) {
          if (inst->getKind() == riscv::Instruction::kCall) {
            auto callInst = dynamic_cast<riscv::CallInst *>(inst.get());
            stackTable->initActiveAtCallInst(callInst);
            // 考虑到call指令为块的第一条指令的情况，第一个判断应为<=而不是<
            for (const auto &symbol : allocator.getFunctionSymbolMap().at(function)) {
              if (!allocator.isSymbolSpilled(symbol)) {
                for (const auto &interval : allocator.getAllocaResult().at(symbol)) {
                  if (interval->left <= 2 * pos + 1 && interval->right >= 2 * pos + 1) {
                    stackTable->addActiveAtCallInst(callInst, interval->physicRegister);
                  }
                }
              }
            }
          }
          pos++;
        }
      }
    }
  }
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto stackTable = pSelector->getStackTable(function);
      stackTable->endSpillInsert();
    }
  }
}
// 用物理寄存器替换符号寄存器
void CodeGenerater::replaceReg() {
  auto pModule = pSelector->getModule();
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      for (const auto &block : functionItem.second->getBlocks()) {
        auto pos = allocator.getBlockList().at(block.get());
        for (const auto &inst : block->getInstructions()) {
          auto dest = inst->getDestination();
          auto sources = inst->getSources();

          auto symbolDest = dynamic_cast<riscv::SymRegister *>(dest);
          if (symbolDest != nullptr) {
            auto newReg = getPhyReg(2 * pos + 1, symbolDest);
            if (newReg != nullptr) {
              inst->setDestination(newReg);
            }
          }

          for (unsigned i = 0; i < sources.size(); i++) {
            auto symbolSource = dynamic_cast<riscv::SymRegister *>(inst->getSource(i));
            if (symbolSource != nullptr) {
              auto newReg = getPhyReg(2 * pos, symbolSource);
              if (newReg != nullptr) {
                inst->setSource(i, newReg);
              }
            }
          }
          pos++;
        }
      }
    }
  }
}
// 初始化插入转移指令所需信息
void CodeGenerater::initMove(
    riscv::Function *function, std::map<unsigned, riscv::BasicBlock::iterator> &postionInstIterMap,
    std::map<unsigned, std::map<riscv::PhyRegister *, riscv::PhyRegister *>> &inBlockMoveMap,
    std::map<riscv::BasicBlock *, std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>>
        &betweenBlockMove) {
  auto comp = [](const riscv::ExtendedLinearScan::ResultInterval *elem1,
                 const riscv::ExtendedLinearScan::ResultInterval *elem2) { return elem1->left < elem2->left; };

  for (const auto &block : function->getBlocks()) {
    betweenBlockMove.emplace(block.get(),
                             std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>{});
    for (const auto &succ : block->getSuccessors()) {
      betweenBlockMove.at(block.get()).emplace(succ, std::map<riscv::PhyRegister *, riscv::PhyRegister *>{});
    }
  }

  for (const auto &symbol : allocator.getFunctionSymbolMap().at(function)) {
    if (!allocator.isSymbolSpilled(symbol)) {
      std::map<riscv::BasicBlock *, std::list<riscv::ExtendedLinearScan::ResultInterval *>> blockIntervalMap;
      for (const auto &interval : allocator.getAllocaResult().at(symbol)) {
        for (const auto &block : function->getBlocks()) {
          auto begin = allocator.getBlockList().at(block.get());
          if (begin <= interval->left / 2 && begin + block->getNumInstructions() > interval->right / 2) {
            if (blockIntervalMap.find(block.get()) == blockIntervalMap.end()) {
              blockIntervalMap.emplace(block.get(), std::list<riscv::ExtendedLinearScan::ResultInterval *>{});
            }
            blockIntervalMap.at(block.get()).emplace_back(interval.get());
            break;
          }
        }
      }
      for (auto &blockIntervalItem : blockIntervalMap) {
        blockIntervalItem.second.sort(comp);
      }

      for (const auto &blockIntervalItem : blockIntervalMap) {
        auto &intervals = blockIntervalItem.second;
        auto iter = intervals.begin();
        for (unsigned i = 0; i < intervals.size() - 1; i++) {
          auto curInterval = *iter;
          auto nextInterval = *std::next(iter);
          auto inst = postionInstIterMap.at(curInterval->right / 2)->get();
          if (curInterval->right + 1 == nextInterval->left &&
              curInterval->physicRegister != nextInterval->physicRegister &&
              nextInterval->physicRegister != inst->getDestination() &&
              curInterval->right / 2 >= 3) {  // 防止其破坏头部块工作
            if (inBlockMoveMap.find(curInterval->right / 2) == inBlockMoveMap.end()) {
              inBlockMoveMap.emplace(curInterval->right / 2, std::map<riscv::PhyRegister *, riscv::PhyRegister *>{});
            }
            inBlockMoveMap.at(curInterval->right / 2)
                .emplace(curInterval->physicRegister, nextInterval->physicRegister);
          }
          iter++;
        }

        auto endInterval = intervals.back();
        if (endInterval->left == endInterval->right) {
          auto curBlock = blockIntervalItem.first;
          for (const auto &sucBlock : curBlock->getSuccessors()) {
            auto iter = blockIntervalMap.find(sucBlock);
            if (iter != blockIntervalMap.end()) {
              auto beginInterval = iter->second.front();
              auto inst = postionInstIterMap.at(endInterval->right / 2)->get();
              if (beginInterval->left == beginInterval->right &&
                  endInterval->physicRegister != beginInterval->physicRegister &&
                  inst->getDestination() != beginInterval->physicRegister) {
                betweenBlockMove.at(curBlock).at(sucBlock).emplace(endInterval->physicRegister,
                                                                   beginInterval->physicRegister);
              }
            }
          }
        }
      }
    }
  }
}
// 在基本块中插入转移指令
// 其关键在于找出链/环
void CodeGenerater::moveRegInBlock(
    std::map<unsigned, riscv::BasicBlock::iterator> &postionInstIterMap,
    std::map<unsigned, std::map<riscv::PhyRegister *, riscv::PhyRegister *>> &inBlockMoveMap) {
  for (const auto &moveItem : inBlockMoveMap) {
    // 找出插入位置
    auto inst = postionInstIterMap.at(moveItem.first)->get();
    auto insertIter = std::next(postionInstIterMap.at(moveItem.first));
    auto &moveGraph = moveItem.second;
    std::set<riscv::PhyRegister *> nodes;
    std::set<riscv::PhyRegister *> roots;
    for (const auto &movePair : moveGraph) {
      nodes.insert(movePair.first);
      nodes.insert(movePair.second);
    }
    roots = nodes;
    for (const auto &movePair : moveGraph) {
      roots.erase(movePair.second);
    }

    // 找出链
    std::list<std::list<riscv::PhyRegister *>> moveChains;
    for (const auto &root : roots) {
      auto reg = root;
      auto iter = moveGraph.find(reg);
      moveChains.emplace_back();
      moveChains.back().emplace_back(root);
      nodes.erase(root);
      while (iter != moveGraph.end()) {
        reg = iter->second;
        iter = moveGraph.find(reg);
        moveChains.back().emplace_back(reg);
        nodes.erase(reg);
      }
    }

    // 找出环
    std::list<std::list<riscv::PhyRegister *>> moveLoops;
    while (!nodes.empty()) {
      auto root = *nodes.begin();
      auto iter = moveGraph.find(root);
      moveLoops.emplace_back();
      moveLoops.back().emplace_back(root);
      nodes.erase(root);
      auto reg = iter->second;
      while (reg != root) {
        iter = moveGraph.find(reg);
        moveLoops.back().emplace_back(reg);
        nodes.erase(reg);
        reg = iter->second;
      }
    }
    auto &builder = pSelector->getBuilder();
    builder.setPosition(inst->getParent(), insertIter);
    // 排除写出对转移指令的影响
    auto instDest = dynamic_cast<riscv::PhyRegister *>(inst->getDestination());
    if (moveGraph.find(instDest) != moveGraph.end()) {
      if (dynamic_cast<riscv::IntPhyRegister *>(instDest) != nullptr) {
        inst->setDestination(intTmpUseRegs.back());
      } else {
        inst->setDestination(floatTmpUseRegs.back());
      }
    }
    // 针对链插入转移指令
    for (const auto &chain : moveChains) {
      auto destIter = chain.rbegin();
      auto intDest = dynamic_cast<riscv::IntPhyRegister *>(*destIter);
      auto floatDest = dynamic_cast<riscv::FloatPhyRegister *>(*destIter);
      if (intDest != nullptr) {
        for (unsigned i = 0; i < chain.size() - 1; i++) {
          auto intSource = dynamic_cast<riscv::IntPhyRegister *>(*std::next(destIter));
          builder.createAddiInst(intDest, intSource, 0);
          intDest = intSource;
          destIter++;
        }
      } else {
        for (unsigned i = 0; i < chain.size() - 1; i++) {
          auto floatSource = dynamic_cast<riscv::FloatPhyRegister *>(*std::next(destIter));
          builder.createFsgnj_sInst(floatDest, floatSource, floatSource);
          floatDest = floatSource;
          destIter++;
        }
      }
    }
    // 针对环插入转移指令
    for (const auto &loop : moveLoops) {
      auto regIter = loop.rbegin();
      auto intReg = dynamic_cast<riscv::IntPhyRegister *>(*regIter);
      auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(*regIter);
      if (intReg != nullptr) {
        for (unsigned i = 0; i < loop.size() - 1; i++) {
          auto antherReg = dynamic_cast<riscv::IntPhyRegister *>(*std::next(regIter));
          builder.createXorInst(intReg, intReg, antherReg);
          builder.createXorInst(antherReg, intReg, antherReg);
          builder.createXorInst(intReg, intReg, antherReg);
          intReg = antherReg;
          regIter++;
        }
      } else {
        auto tmpReg = intTmpUseRegs.back();
        for (unsigned i = 0; i < loop.size() - 1; i++) {
          auto antherReg = dynamic_cast<riscv::FloatPhyRegister *>(*std::next(regIter));
          builder.createFmv_x_wInst(tmpReg, floatReg);
          builder.createFsgnj_sInst(floatReg, antherReg, antherReg);
          builder.createFmv_w_xInst(antherReg, tmpReg);
          floatReg = antherReg;
          regIter++;
        }
      }
    }
    if (moveGraph.find(instDest) != moveGraph.end()) {
      if (dynamic_cast<riscv::IntPhyRegister *>(instDest) != nullptr) {
        builder.createAddiInst(dynamic_cast<riscv::IntPhyRegister *>(instDest), intTmpUseRegs.back(), 0);
      } else {
        builder.createFsgnj_sInst(dynamic_cast<riscv::FloatPhyRegister *>(instDest), floatTmpUseRegs.back(),
                                  floatTmpUseRegs.back());
      }
    }
  }
}
// 在基本块之间插入转移指令
// 注意到插入指令定义在CFG的边上，需要插入额外的块进行转移
// 其他细节与moveRegInBlock类似
void CodeGenerater::moveRegBetweenBlock(
    riscv::Function *function,
    std::map<riscv::BasicBlock *, std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>>
        &betweenBlockMove) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto &builder = pSelector->getBuilder();
  for (const auto &sourceItem : betweenBlockMove) {
    auto sourceBlock = sourceItem.first;
    for (const auto &destItem : sourceItem.second) {
      auto destBlock = destItem.first;
      auto &moveGraph = destItem.second;
      if (!moveGraph.empty()) {
        // 插入新块
        auto newBlock = function->addBasicBlock();
        auto inst = sourceBlock->back().get();
        if (inst->isBInst()) {
          auto bInst = dynamic_cast<riscv::BInst *>(inst);
          if (bInst->getThenBlock() == destBlock) {
            bInst->setThenBlock(newBlock);
          }
        } else if (inst->isJumpInst()) {
          auto jalInst = dynamic_cast<riscv::JInst *>(inst);
          if (jalInst != nullptr) {
            jalInst->setLabel(newBlock);
          }
        }
        sourceBlock->removeSuccessor(destBlock);
        sourceBlock->addSuccessor(newBlock);
        destBlock->removePredecessor(sourceBlock);
        destBlock->addPredecessor(newBlock);
        newBlock->addPredecessor(sourceBlock);
        newBlock->addSuccessor(destBlock);
        builder.setPosition(newBlock, newBlock->end());
        builder.createJalInst(zero, destBlock);

        std::set<riscv::PhyRegister *> nodes;
        std::set<riscv::PhyRegister *> roots;
        for (const auto &movePair : moveGraph) {
          nodes.insert(movePair.first);
          nodes.insert(movePair.second);
        }
        roots = nodes;
        for (const auto &movePair : moveGraph) {
          roots.erase(movePair.second);
        }

        // 找出链
        std::list<std::list<riscv::PhyRegister *>> moveChains;
        for (const auto &root : roots) {
          auto reg = root;
          auto iter = moveGraph.find(reg);
          moveChains.emplace_back();
          moveChains.back().emplace_back(root);
          nodes.erase(root);
          while (iter != moveGraph.end()) {
            reg = iter->second;
            iter = moveGraph.find(reg);
            moveChains.back().emplace_back(reg);
            nodes.erase(reg);
          }
        }

        // 找出环
        std::list<std::list<riscv::PhyRegister *>> moveLoops;
        while (!nodes.empty()) {
          auto root = *nodes.begin();
          auto iter = moveGraph.find(root);
          moveLoops.emplace_back();
          moveLoops.back().emplace_back(root);
          nodes.erase(root);
          auto reg = iter->second;
          while (reg != root) {
            iter = moveGraph.find(reg);
            moveLoops.back().emplace_back(reg);
            nodes.erase(reg);
            reg = iter->second;
          }
        }

        builder.setPosition(newBlock, std::prev(newBlock->end()));
        // 针对链插入转移指令
        for (const auto &chain : moveChains) {
          auto destIter = chain.rbegin();
          auto intDest = dynamic_cast<riscv::IntPhyRegister *>(*destIter);
          auto floatDest = dynamic_cast<riscv::FloatPhyRegister *>(*destIter);
          if (intDest != nullptr) {
            for (unsigned i = 0; i < chain.size() - 1; i++) {
              auto intSource = dynamic_cast<riscv::IntPhyRegister *>(*std::next(destIter));
              builder.createAddiInst(intDest, intSource, 0);
              intDest = intSource;
              destIter++;
            }
          } else {
            for (unsigned i = 0; i < chain.size() - 1; i++) {
              auto floatSource = dynamic_cast<riscv::FloatPhyRegister *>(*std::next(destIter));
              builder.createFsgnj_sInst(floatDest, floatSource, floatSource);
              floatDest = floatSource;
              destIter++;
            }
          }
        }
        // 针对环插入转移指令
        for (const auto &loop : moveLoops) {
          auto regIter = loop.rbegin();
          auto intReg = dynamic_cast<riscv::IntPhyRegister *>(*regIter);
          auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(*regIter);
          if (intReg != nullptr) {
            for (unsigned i = 0; i < loop.size() - 1; i++) {
              auto antherReg = dynamic_cast<riscv::IntPhyRegister *>(*std::next(regIter));
              builder.createXorInst(intReg, intReg, antherReg);
              builder.createXorInst(antherReg, intReg, antherReg);
              builder.createXorInst(intReg, intReg, antherReg);
              intReg = antherReg;
              regIter++;
            }
          } else {
            auto tmpReg = intTmpUseRegs.back();
            for (unsigned i = 0; i < loop.size() - 1; i++) {
              auto antherReg = dynamic_cast<riscv::FloatPhyRegister *>(*std::next(regIter));
              builder.createFmv_x_wInst(tmpReg, floatReg);
              builder.createFsgnj_sInst(floatReg, antherReg, antherReg);
              builder.createFmv_w_xInst(antherReg, tmpReg);
              floatReg = antherReg;
              regIter++;
            }
          }
        }
      }
    }
  }
}
// 插入寄存器转移指令
void CodeGenerater::moveReg() {
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      std::map<unsigned, riscv::BasicBlock::iterator> positionInstIterMap;
      for (const auto &block : functionItem.second->getBlocks()) {
        auto iter = block->begin();
        auto begin = allocator.getBlockList().at(block.get());
        for (unsigned i = 0; i < block->getNumInstructions(); i++) {
          positionInstIterMap.emplace(begin + i, iter);
          iter++;
        }
      }
      std::map<unsigned, std::map<riscv::PhyRegister *, riscv::PhyRegister *>> inBlockMoveMap;
      std::map<riscv::BasicBlock *, std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>>
          betweenBlockMove;
      initMove(functionItem.second.get(), positionInstIterMap, inBlockMoveMap, betweenBlockMove);
      moveRegInBlock(positionInstIterMap, inBlockMoveMap);
      moveRegBetweenBlock(functionItem.second.get(), betweenBlockMove);
    }
  }
}
// 用来处理函数参数的传递(需要加强测试)
void CodeGenerater::callParamPass() {
  auto pModule = pSelector->getModule();
  auto &builder = pSelector->getBuilder();
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  for (const auto &functionItem : pModule->getFunctions()) {
    auto function = functionItem.second.get();
    auto callerStackTable = pSelector->getStackTable(function);
    if (!function->isExternalFucntion()) {
      for (const auto &block : function->getBlocks()) {
        auto inserter = block->begin();
        while (inserter != block->end()) {
          auto inst = inserter->get();
          if (inst->getKind() == riscv::Instruction::kCall) {
            // 保存活跃的传参寄存器
            auto callInst = dynamic_cast<riscv::CallInst *>(inst);
            auto calleeStackTable = pSelector->getStackTable(dynamic_cast<riscv::Function *>(callInst->getLabel()));
            auto notSpillParamRegs = calleeStackTable->getNotSpillParamRegs();
            builder.setPosition(block.get(), std::prev(inserter, notSpillParamRegs.size()));
            auto reserveRegs = callerStackTable->getCallerReserveRegMap(callInst);
            for (const auto &regItem : reserveRegs) {
              if (notSpillParamRegs.find(regItem.first) != notSpillParamRegs.end()) {
                auto intReg = dynamic_cast<riscv::IntPhyRegister *>(regItem.first);
                auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(regItem.first);
                auto offset = regItem.second;
                if (intReg != nullptr) {
                  if (offset < 2047) {
                    builder.createSdInst(intReg, sp, offset);
                  } else {
                    auto tmpReg = intTmpUseRegs.back();
                    builder.createLiInst(tmpReg, offset);
                    builder.createAddInst(tmpReg, tmpReg, sp);
                    builder.createSdInst(intReg, tmpReg, 0);
                  }
                } else {
                  if (offset < 2047) {
                    builder.createFswInst(floatReg, sp, offset);
                  } else {
                    auto tmpReg = intTmpUseRegs.back();
                    builder.createLiInst(tmpReg, offset);
                    builder.createAddInst(tmpReg, tmpReg, sp);
                    builder.createFswInst(floatReg, tmpReg, 0);
                  }
                }
              }
            }

            // 初始化传参映射(倒序)
            std::map<riscv::PhyRegister *, riscv::PhyRegister *> reg2RegRevMap;
            std::map<riscv::PhyRegister *, riscv::SymRegister *> sym2RegRevMap;
            std::map<riscv::PhyRegister *, uint64_t> imm2RegRevMap;
            std::set<riscv::PhyRegister *> regs;
            auto posIter = inserter;
            for (unsigned i = 0; i < notSpillParamRegs.size(); i++) {
              posIter--;
              auto inst = posIter->get();
              auto liInst = dynamic_cast<riscv::LiInst *>(inst);
              auto dest = dynamic_cast<riscv::PhyRegister *>(inst->getDestination());
              regs.insert(dest);
              if (liInst != nullptr) {
                imm2RegRevMap.emplace(dest, liInst->getImm());
              } else {
                auto source = inst->getSource(0);
                auto symSource = dynamic_cast<riscv::SymRegister *>(source);
                auto phySource = dynamic_cast<riscv::PhyRegister *>(source);
                if (symSource != nullptr) {
                  sym2RegRevMap.emplace(dest, symSource);
                } else {
                  if (phySource != dest) {
                    reg2RegRevMap.emplace(dest, phySource);
                    regs.insert(phySource);
                  }
                }
              }
            }
            // 初始化传参映射(顺序)
            std::map<riscv::PhyRegister *, std::set<riscv::PhyRegister *>> reg2RegMap;
            std::map<riscv::SymRegister *, std::set<riscv::PhyRegister *>> sym2RegMap;
            std::map<uint64_t, std::set<riscv::PhyRegister *>> imm2RegMap;
            for (const auto &reg2RegItem : reg2RegRevMap) {
              if (reg2RegMap.find(reg2RegItem.second) == reg2RegMap.end()) {
                reg2RegMap.emplace(reg2RegItem.second, std::set<riscv::PhyRegister *>{});
              }
              reg2RegMap.at(reg2RegItem.second).insert(reg2RegItem.first);
            }
            for (const auto &sym2RegItem : sym2RegRevMap) {
              if (sym2RegMap.find(sym2RegItem.second) == sym2RegMap.end()) {
                sym2RegMap.emplace(sym2RegItem.second, std::set<riscv::PhyRegister *>{});
              }
              sym2RegMap.at(sym2RegItem.second).insert(sym2RegItem.first);
            }
            for (const auto &imm2RegItem : imm2RegRevMap) {
              if (imm2RegMap.find(imm2RegItem.second) == imm2RegMap.end()) {
                imm2RegMap.emplace(imm2RegItem.second, std::set<riscv::PhyRegister *>{});
              }
              imm2RegMap.at(imm2RegItem.second).insert(imm2RegItem.first);
            }

            std::set<riscv::PhyRegister *> notRootRegs;
            for (const auto &regItem : reg2RegRevMap) {
              notRootRegs.insert(regItem.first);
            }

            // 找出环和树
            std::set<riscv::PhyRegister *> remRegs = regs;
            std::list<std::list<riscv::PhyRegister *>> loops;
            std::set<riscv::PhyRegister *> loopRegs;
            std::set<riscv::PhyRegister *> regTreeRoots;
            while (!remRegs.empty()) {
              auto curReg = *remRegs.begin();
              std::list<riscv::PhyRegister *> loop;
              std::map<riscv::PhyRegister *, std::list<riscv::PhyRegister *>::iterator> record;
              while (record.find(curReg) == record.end() && notRootRegs.find(curReg) != notRootRegs.end()) {
                loop.emplace_front(curReg);
                record.emplace(curReg, loop.begin());
                curReg = reg2RegRevMap.at(curReg);
              }
              std::stack<riscv::PhyRegister *> extends;
              if (record.find(curReg) != record.end()) {
                loop.erase(std::next(record.at(curReg)), loop.end());
                loops.emplace_back(loop);
                std::set<riscv::PhyRegister *> rootRegs;
                for (const auto &reg : loop) {
                  loopRegs.insert(reg);
                  remRegs.erase(reg);
                  if (reg2RegMap.at(reg).size() > 1) {
                    regTreeRoots.insert(reg);
                    rootRegs.insert(reg);
                  }
                }
                for (const auto &root : rootRegs) {
                  for (const auto &child : reg2RegMap.at(root)) {
                    if (loopRegs.find(child) == loopRegs.end()) {
                      extends.push(child);
                    }
                  }
                }
              } else {
                regTreeRoots.insert(curReg);
                extends.push(curReg);
              }
              while (!extends.empty()) {
                auto extend = extends.top();
                extends.pop();
                remRegs.erase(extend);
                if (reg2RegMap.find(extend) != reg2RegMap.end()) {
                  for (const auto &child : reg2RegMap.at(extend)) {
                    extends.push(child);
                  }
                }
              }
            }

            // 移除原有的转移代码
            std::list<riscv::BasicBlock::iterator> delIters;
            posIter = inserter;
            for (unsigned i = 0; i < notSpillParamRegs.size(); i++) {
              posIter--;
              delIters.emplace_back(posIter);
            }
            for (const auto &delIter : delIters) {
              block->removeInst(delIter);
            }
            // 生成新的转移代码(可能有问题)
            // 先针对树生成转移代码，再针对环生成交换代码
            builder.setPosition(block.get(), inserter);
            remRegs = regs;
            std::queue<riscv::PhyRegister *> BFGqueue;
            std::stack<riscv::PhyRegister *> BFGstack;
            for (const auto &root : regTreeRoots) {
              if (reg2RegMap.find(root) != reg2RegMap.end()) {
                for (const auto &child : reg2RegMap.at(root)) {
                  if (loopRegs.find(child) == loopRegs.end()) {
                    BFGqueue.push(child);
                  }
                }
              }
              while (!BFGqueue.empty()) {
                auto reg = BFGqueue.front();
                BFGqueue.pop();
                BFGstack.push(reg);
                if (reg2RegMap.find(reg) != reg2RegMap.end()) {
                  for (const auto &child : reg2RegMap.at(reg)) {
                    BFGqueue.push(child);
                  }
                }
              }
              while (!BFGstack.empty()) {
                auto reg = BFGstack.top();
                BFGstack.pop();
                auto parent = reg2RegRevMap.at(reg);
                auto intReg = dynamic_cast<riscv::IntPhyRegister *>(reg);
                auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(reg);
                remRegs.erase(reg);
                if (intReg != nullptr) {
                  builder.createAddiInst(intReg, dynamic_cast<riscv::IntPhyRegister *>(parent), 0);
                } else {
                  builder.createFsgnj_sInst(floatReg, dynamic_cast<riscv::FloatPhyRegister *>(parent),
                                            dynamic_cast<riscv::FloatPhyRegister *>(parent));
                }
              }
            }
            for (const auto &sym2RegItem : sym2RegMap) {
              auto sym = sym2RegItem.first;
              for (const auto &root : sym2RegItem.second) {
                auto intReg = dynamic_cast<riscv::IntPhyRegister *>(root);
                auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(root);
                if (intReg != nullptr) {
                  builder.createAddiInst(intReg, dynamic_cast<riscv::IntSymRegister *>(sym), 0);
                } else {
                  builder.createFsgnj_sInst(floatReg, dynamic_cast<riscv::FloatSymRegister *>(sym),
                                            dynamic_cast<riscv::FloatSymRegister *>(sym));
                }
              }
            }
            for (const auto &imm2RegItem : imm2RegMap) {
              auto imm = imm2RegItem.first;
              for (const auto &root : imm2RegItem.second) {
                builder.createLiInst(dynamic_cast<riscv::IntPhyRegister *>(root), imm);
              }
            }
            for (const auto &loop : loops) {
              auto reg = loop.front();
              auto intReg = dynamic_cast<riscv::IntPhyRegister *>(reg);
              auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(reg);
              auto iter = std::next(loop.begin());
              if (intReg != nullptr) {
                while (iter != loop.end()) {
                  auto anotherReg = dynamic_cast<riscv::IntPhyRegister *>(*iter);
                  builder.createXorInst(intReg, anotherReg, intReg);
                  builder.createXorInst(anotherReg, anotherReg, intReg);
                  builder.createXorInst(intReg, intReg, anotherReg);
                  iter++;
                }
              } else {
                auto tmpReg = intTmpUseRegs.back();
                while (iter != loop.end()) {
                  auto anotherReg = dynamic_cast<riscv::FloatPhyRegister *>(*iter);
                  builder.createFmv_x_wInst(tmpReg, floatReg);
                  builder.createFsgnj_sInst(floatReg, anotherReg, anotherReg);
                  builder.createFmv_w_xInst(anotherReg, tmpReg);
                  iter++;
                }
              }
            }

            // 还原活跃的传参寄存器
            posIter = std::next(inserter);
            auto callee = dynamic_cast<riscv::Function *>(callInst->getLabel());
            if (callee->getReturnType() == riscv::Function::VOID) {
              builder.setPosition(block.get(), posIter);
            } else {
              builder.setPosition(block.get(), std::next(posIter));
            }
            for (const auto &regItem : reserveRegs) {
              if (notSpillParamRegs.find(regItem.first) != notSpillParamRegs.end()) {
                auto intReg = dynamic_cast<riscv::IntPhyRegister *>(regItem.first);
                auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(regItem.first);
                auto offset = regItem.second;
                if (intReg != nullptr) {
                  if (offset < 2047) {
                    builder.createLdInst(intReg, sp, offset);
                  } else {
                    auto tmpReg = intTmpUseRegs.back();
                    builder.createLiInst(tmpReg, offset);
                    builder.createAddInst(tmpReg, tmpReg, sp);
                    builder.createLdInst(intReg, tmpReg, 0);
                  }
                } else {
                  if (offset < 2047) {
                    builder.createFlwInst(floatReg, sp, offset);
                  } else {
                    auto tmpReg = intTmpUseRegs.back();
                    builder.createLiInst(tmpReg, offset);
                    builder.createAddInst(tmpReg, tmpReg, sp);
                    builder.createFlwInst(floatReg, tmpReg, 0);
                  }
                }
              }
            }
          }
          inserter++;
        }
      }
    }
  }
}
// 为溢出符号变量插入溢出代码
void CodeGenerater::insertSpillReg() {
  auto pModule = pSelector->getModule();
  auto &builder = pSelector->getBuilder();
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      auto stackTable = pSelector->getStackTable(functionItem.second.get());
      for (const auto &block : functionItem.second->getBlocks()) {
        auto instIter = block->begin();
        while (instIter != block->end()) {
          auto inst = instIter->get();
          auto intSpillReg = intTmpUseRegs;
          auto floatSpillReg = floatTmpUseRegs;
          auto dest = inst->getDestination();
          auto sources = inst->getSources();
          unsigned sourceIndex = 0;
          for (const auto &source : sources) {
            auto symbolSource = dynamic_cast<riscv::SymRegister *>(source);
            if (symbolSource != nullptr && allocator.isSymbolSpilled(symbolSource)) {
              auto spillAddr = stackTable->getSpillAddr(symbolSource);
              auto intSymbolSource = dynamic_cast<riscv::IntSymRegister *>(symbolSource);
              builder.setPosition(block.get(), instIter);
              if (intSymbolSource != nullptr) {
                auto resultReg = intSpillReg.back();
                intSpillReg.pop_back();
                if (spillAddr < 2048) {
                  if (intSymbolSource->is64Value()) {
                    builder.createLdInst(resultReg, fp, -spillAddr);
                  } else {
                    builder.createLwInst(resultReg, fp, -spillAddr);
                  }
                } else {
                  builder.createLiInst(resultReg, spillAddr);
                  builder.createSubInst(resultReg, fp, resultReg);
                  if (intSymbolSource->is64Value()) {
                    builder.createLdInst(resultReg, resultReg, 0);
                  } else {
                    builder.createLwInst(resultReg, resultReg, 0);
                  }
                }
                inst->setSource(sourceIndex, resultReg);
              } else {
                auto resultReg = floatSpillReg.back();
                floatSpillReg.pop_back();
                if (spillAddr < 2048) {
                  builder.createFlwInst(resultReg, fp, -spillAddr);
                } else {
                  auto addrReg = intSpillReg.back();
                  intSpillReg.pop_back();
                  builder.createLiInst(addrReg, spillAddr);
                  builder.createSubInst(addrReg, fp, addrReg);
                  builder.createFlwInst(resultReg, addrReg, 0);
                  intSpillReg.push_back(addrReg);
                }
                inst->setSource(sourceIndex, resultReg);
              }
            }
            sourceIndex++;
          }

          instIter++;
          intSpillReg = intTmpUseRegs;
          floatSpillReg = floatTmpUseRegs;
          auto symbolDest = dynamic_cast<riscv::SymRegister *>(dest);
          if (symbolDest != nullptr &&
              allocator.getAllocaResult().find(symbolDest) != allocator.getAllocaResult().end() &&
              allocator.isSymbolSpilled(symbolDest)) {
            auto spillAddr = stackTable->getSpillAddr(symbolDest);
            auto intSymbolDest = dynamic_cast<riscv::IntSymRegister *>(symbolDest);
            builder.setPosition(block.get(), instIter);
            if (intSymbolDest != nullptr) {
              auto resultReg = intSpillReg.back();
              intSpillReg.pop_back();
              inst->setDestination(resultReg);
              if (spillAddr < 2048) {
                if (intSymbolDest->is64Value()) {
                  builder.createSdInst(resultReg, fp, -spillAddr);
                } else {
                  builder.createSwInst(resultReg, fp, -spillAddr);
                }
              } else {
                auto addrReg = intSpillReg.back();
                intSpillReg.pop_back();
                builder.createLiInst(addrReg, spillAddr);
                builder.createSubInst(addrReg, fp, addrReg);
                if (intSymbolDest->is64Value()) {
                  builder.createSdInst(resultReg, addrReg, 0);
                } else {
                  builder.createSwInst(resultReg, addrReg, 0);
                }
                intSpillReg.push_back(addrReg);
              }
            } else {
              auto resultReg = floatSpillReg.back();
              intSpillReg.pop_back();
              inst->setDestination(resultReg);
              if (spillAddr < 2048) {
                builder.createFswInst(resultReg, fp, -spillAddr);
              } else {
                auto addrReg = intSpillReg.back();
                intSpillReg.pop_back();
                builder.createLiInst(addrReg, spillAddr);
                builder.createSubInst(addrReg, fp, addrReg);
                builder.createFswInst(resultReg, addrReg, 0);
                intSpillReg.push_back(addrReg);
              }
            }
          }
        }
      }
    }
  }
}
// 保存被调用者保存的寄存器
void CodeGenerater::reserveCalleeReg() {
  auto &builder = pSelector->getBuilder();
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto entryBlock = function->getEntryBlock();
      auto stackTable = pSelector->getStackTable(function);
      builder.setPosition(entryBlock, std::next(entryBlock->begin(), 4));
      for (const auto &calleeRegItem : stackTable->getCalleeReserveRegMap()) {
        auto intReg = dynamic_cast<riscv::IntPhyRegister *>(calleeRegItem.first);
        auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(calleeRegItem.first);
        auto offset = calleeRegItem.second;
        if (intReg != nullptr) {
          if (offset > 2048) {
            auto tmpReg = intTmpUseRegs.back();
            builder.createLiInst(tmpReg, offset);
            builder.createSubInst(tmpReg, fp, tmpReg);
            builder.createSdInst(intReg, tmpReg, 0);
          } else {
            builder.createSdInst(intReg, fp, -offset);
          }
        } else {
          if (offset > 2048) {
            auto tmpReg = intTmpUseRegs.back();
            builder.createLiInst(tmpReg, offset);
            builder.createSubInst(tmpReg, fp, tmpReg);
            builder.createFswInst(floatReg, tmpReg, 0);
          } else {
            builder.createFswInst(floatReg, fp, -offset);
          }
        }
      }
      for (const auto &block : function->getBlocks()) {
        if (block->back()->getKind() == riscv::Instruction::kJalr) {
          builder.setPosition(block.get(), std::prev(block->end(), 4));
          for (const auto &calleeRegItem : stackTable->getCalleeReserveRegMap()) {
            auto intReg = dynamic_cast<riscv::IntPhyRegister *>(calleeRegItem.first);
            auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(calleeRegItem.first);
            auto offset = calleeRegItem.second;
            if (intReg != nullptr) {
              if (offset > 2048) {
                auto tmpReg = intTmpUseRegs.back();
                builder.createLiInst(tmpReg, offset);
                builder.createSubInst(tmpReg, fp, tmpReg);
                builder.createLdInst(intReg, tmpReg, 0);
              } else {
                builder.createLdInst(intReg, fp, -offset);
              }
            } else {
              if (offset > 2048) {
                auto tmpReg = intTmpUseRegs.back();
                builder.createLiInst(tmpReg, offset);
                builder.createSubInst(tmpReg, fp, tmpReg);
                builder.createFlwInst(floatReg, tmpReg, 0);
              } else {
                builder.createFlwInst(floatReg, fp, -offset);
              }
            }
          }
        }
      }
    }
  }
}
// 保存调用者保存的寄存器
// 更详细的分析要用到call指令处的活跃信息
// 需要结合函数的传参进行考虑(一部分在传参前保存，一部分在传参后保存)
// 注意有返回值的call指令
void CodeGenerater::reserveCallerReg() {
  auto &builder = pSelector->getBuilder();
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      for (const auto &block : function->getBlocks()) {
        auto instIter = block->begin();
        while (instIter != block->end()) {
          auto inst = instIter->get();
          auto nextIter = std::next(instIter);
          if (inst->getKind() == riscv::Instruction::kCall) {
            auto callInst = dynamic_cast<riscv::CallInst *>(inst);
            auto callee = dynamic_cast<riscv::Function *>(callInst->getLabel());
            auto callerStackTable = pSelector->getStackTable(function);
            auto calleeStackTable = pSelector->getStackTable(callee);
            auto &notSpillParamRegs = calleeStackTable->getNotSpillParamRegs();
            builder.setPosition(block.get(), instIter);
            for (const auto &callerRegItem : callerStackTable->getCallerReserveRegMap(callInst)) {
              if (notSpillParamRegs.find(callerRegItem.first) != notSpillParamRegs.end()) {
                continue;
              }
              auto intReg = dynamic_cast<riscv::IntPhyRegister *>(callerRegItem.first);
              auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(callerRegItem.first);
              auto offset = callerRegItem.second;
              if (intReg != nullptr) {
                if (offset > 2047) {
                  auto tmpReg = intTmpUseRegs.back();
                  builder.createLiInst(tmpReg, offset);
                  builder.createAddInst(tmpReg, sp, tmpReg);
                  builder.createSdInst(intReg, tmpReg, 0);
                } else {
                  builder.createSdInst(intReg, sp, offset);
                }
              } else {
                if (offset > 2048) {
                  auto tmpReg = intTmpUseRegs.back();
                  builder.createLiInst(tmpReg, offset);
                  builder.createAddInst(tmpReg, sp, tmpReg);
                  builder.createFswInst(floatReg, tmpReg, 0);
                } else {
                  builder.createFswInst(floatReg, sp, offset);
                }
              }
            }
            if (callee->getReturnType() == riscv::Function::VOID) {
              builder.setPosition(block.get(), nextIter);
            } else {
              builder.setPosition(block.get(), std::next(nextIter));
            }
            for (const auto &callerRegItem : callerStackTable->getCallerReserveRegMap(callInst)) {
              if (notSpillParamRegs.find(callerRegItem.first) != notSpillParamRegs.end()) {
                continue;
              }
              auto intReg = dynamic_cast<riscv::IntPhyRegister *>(callerRegItem.first);
              auto floatReg = dynamic_cast<riscv::FloatPhyRegister *>(callerRegItem.first);
              auto offset = callerRegItem.second;
              if (intReg != nullptr) {
                if (offset > 2047) {
                  auto tmpReg = intTmpUseRegs.back();
                  builder.createLiInst(tmpReg, offset);
                  builder.createAddInst(tmpReg, sp, tmpReg);
                  builder.createLdInst(intReg, tmpReg, 0);
                } else {
                  builder.createLdInst(intReg, sp, offset);
                }
              } else {
                if (offset > 2048) {
                  auto tmpReg = intTmpUseRegs.back();
                  builder.createLiInst(tmpReg, offset);
                  builder.createAddInst(tmpReg, sp, tmpReg);
                  builder.createFlwInst(floatReg, tmpReg, 0);
                } else {
                  builder.createFlwInst(floatReg, sp, offset);
                }
              }
            }
          }
          instIter = nextIter;
        }
      }
    }
  }
}
// 设置栈大小
void CodeGenerater::setStackSize() {
  auto &builder = pSelector->getBuilder();
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto offset = pSelector->getStackTable(function)->getStackSize();
      auto entryBlock = function->getEntryBlock();
      auto iter = std::next(entryBlock->begin(), 3);
      auto addiInst = dynamic_cast<riscv::IInst *>(iter->get());
      if (addiInst != nullptr) {
        if (offset < 2048) {
          addiInst->setImm(-offset);
        } else {
          auto nextIter = std::next(iter);
          entryBlock->removeInst(iter);
          auto tmpReg = intTmpUseRegs.back();
          builder.setPosition(entryBlock, nextIter);
          builder.createLiInst(tmpReg, offset);
          builder.createSubInst(sp, sp, tmpReg);
        }
      }
      for (const auto &block : function->getBlocks()) {
        if (block->back()->getKind() == riscv::Instruction::kJalr) {
          iter = std::prev(block->end(), 2);
          addiInst = dynamic_cast<riscv::IInst *>(iter->get());
          if (offset < 2047) {
            addiInst->setImm(offset);
          } else {
            auto nextIter = std::next(iter);
            entryBlock->removeInst(iter);
            auto tmpReg = intTmpUseRegs.back();
            builder.setPosition(entryBlock, nextIter);
            builder.createLiInst(tmpReg, offset);
            builder.createAddInst(sp, sp, tmpReg);
          }
        }
      }
    }
  }
}
// 去除无用指令
void CodeGenerater::simplify() {
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      for (const auto &block : function->getBlocks()) {
        auto iter = block->begin();
        while (iter != block->end()) {
          auto inst = iter->get();
          auto nextIter = std::next(iter);
          auto dest = inst->getDestination();
          auto sources = inst->getSources();

          if (dynamic_cast<riscv::SymRegister *>(dest) != nullptr) {
            block->removeInst(iter);
          } else {
            for (const auto &source : sources) {
              if (dynamic_cast<riscv::SymRegister *>(source) != nullptr) {
                block->removeInst(iter);
                break;
              }
            }
          }
          iter = nextIter;
        }
      }
    }
  }
}
// 重命名starttime()、stoptime()
void CodeGenerater::renameFunctions() {
  auto pModule = pSelector->getModule();
  auto &funtions = pModule->getFunctions();
  if (funtions.find("starttime") != funtions.end()) {
    funtions.find("starttime")->second->setName("_sysy_starttime");
  }
  if (funtions.find("stoptime") != funtions.end()) {
    funtions.find("stoptime")->second->setName("_sysy_stoptime");
  }
}

// 合并一一对应关系的基本块，减少后续处理开销，并删除死块
void CodeGenerater::combBlocks() {
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      std::stack<riscv::BasicBlock *> blockStack;
      std::list<riscv::BasicBlock *> blockList;
      std::map<riscv::BasicBlock *, bool> isAdded;
      for (const auto &block : function->getBlocks()) {
        isAdded.emplace(block.get(), false);
      }

      auto entryBlock = function->getEntryBlock();
      blockStack.push(entryBlock);
      isAdded.at(entryBlock) = true;
      while (!blockStack.empty()) {
        auto bk = blockStack.top();
        blockStack.pop();
        blockList.emplace_back(bk);
        for (const auto &succ : bk->getSuccessors()) {
          if (!isAdded.at(succ)) {
            blockStack.push(succ);
            isAdded.at(succ) = true;
          }
        }
      }

      auto &blocks = function->getBlocks();
      auto iter = blocks.begin();
      while (iter != blocks.end()) {
        if (!isAdded.at(iter->get())) {
          iter = blocks.erase(iter);
        } else {
          iter++;
        }
      }

      while (!blockList.empty()) {
        auto bk = blockList.back();
        blockList.pop_back();
        if (!blockList.empty()) {
          auto predBk = blockList.back();
          if (bk->getNumPredecessors() == 1 && predBk->getNumSuccessors() == 1 &&
              *(bk->getPredecessors().begin()) == predBk) {
            for (const auto &succ : bk->getSuccessors()) {
              predBk->addSuccessor(succ);
              succ->addPredecessor(predBk);
            }

            if (predBk->back()->getKind() == riscv::Instruction::kJal) {
              predBk->removeInst(--(predBk->end()));
            }
            predBk->mergeBlock(bk);
          }
        }
      }
    }
  }
}
void CodeGenerater::sortAndRenameBlocks() {
  uint64_t begin = 0;
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      function->sortBlocks();
      begin = function->renameBlocks(begin);
    }
  }
}
void CodeGenerater::deleteAfterSort() {
  for (const auto &functionItem : pSelector->getModule()->getFunctions()) {
    auto function = functionItem.second.get();
    if (!function->isExternalFucntion()) {
      auto iter = function->getBlocks().begin();
      while (iter != function->getBlocks().end()) {
        auto block = iter->get();
        auto endInst = block->back().get();
        iter++;
        if (endInst->getKind() == riscv::Instruction::kJal) {
          auto targetBlock = dynamic_cast<riscv::JInst *>(endInst)->getLabel();
          if (iter != function->getBlocks().end() && iter->get() == targetBlock) {
            block->removeInst(--(block->end()));
          }
        }
      }
    }
  }
}
}  // namespace mid2end
