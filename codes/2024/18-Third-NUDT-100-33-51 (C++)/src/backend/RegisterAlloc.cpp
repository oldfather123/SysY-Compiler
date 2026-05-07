#include "../include/backend/RegisterAlloc.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <stack>
#include <utility>
#include <vector>
#include "../include/backend/Mid2End.h"
#include "../include/backend/Riscv.h"
#include "../include/backend/RiscvBuilder.h"

/**
 * @file RegisterAlloc.h
 *
 * 定义了寄存器分配的源文件
 */
// 可能存在的问题：函数体中参数的取用
namespace riscv {
const std::list<IntPhyRegister *> ExtendedLinearScan::intPhyRegisterPool = {
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::ra),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::t2),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s1),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::a0),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::a1),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::a2),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::a3),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::a4),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::a5),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::a6),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::a7),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::s2),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s3),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::s4),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s5),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::s6),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s7),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::s8),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s9),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::s10),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::s11), IntPhyRegister::getIntPhyRegister(IntPhyRegister::t3),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::t4),  IntPhyRegister::getIntPhyRegister(IntPhyRegister::t5),
    IntPhyRegister::getIntPhyRegister(IntPhyRegister::t6)};
const std::list<FloatPhyRegister *> ExtendedLinearScan::floatPhyRegisterPool = {
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft3),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft4),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft5),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft6),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft7),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs0),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs1),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa0),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa1),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa2),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa3),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa4),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa5),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa6),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fa7),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs2),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs3),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs4),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs5),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs6),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs7),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs8),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs9),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs10),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::fs11),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft8),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft9),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft10),
    FloatPhyRegister::getFloatPhyRegister(FloatPhyRegister::ft11),
};
auto ExtendedLinearScan::findOrEmplaceEndPoints(
    unsigned inputPosition, BasicBlock *block,
    const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable) -> EndPoint * {
  auto iter = positonEndPointMap.find(inputPosition);
  if (iter != positonEndPointMap.end()) {
    return iter->second.get();
  }
  auto result = new EndPoint;
  result->position = inputPosition;
  positonEndPointMap.emplace(inputPosition, result);
  auto begin = blockList[block];
  result->setActiveSet(activeTable.find(block)->second.at((inputPosition + 1) / 2 - begin));
  return result;
}
auto ExtendedLinearScan::getMoveSource(unsigned position, SymRegister *targetSymbol) const -> SymRegister * {
  auto inst = instructionList.at(position);
  using Kind = Instruction::InstKind;
  auto zero = IntPhyRegister::getIntPhyRegister(IntPhyRegister::zero);
  switch (inst->getKind()) {
    case Kind::kSll:
    case Kind::kSrl:
    case Kind::kSra:
    case Kind::kSllw:
    case Kind::kSrlw:
    case Kind::kSraw:
    case Kind::kSub:
    case Kind::kSubw: {
      if (inst->getDestination() == targetSymbol && inst->getSource(1) == zero) {
        return dynamic_cast<SymRegister *>(inst->getSource(0));
      }
      return nullptr;
    }
    case Kind::kSlli:
    case Kind::kSrli:
    case Kind::kSrai:
    case Kind::kSlliw:
    case Kind::kSrliw:
    case Kind::kSraiw:
    case Kind::kAddi:
    case Kind::kAddiw:
    case Kind::kXori:
    case Kind::kOri: {
      auto iInst = dynamic_cast<IInst *>(inst);
      if (iInst->getDestination() == targetSymbol && iInst->getImm() == 0) {
        return dynamic_cast<SymRegister *>(iInst->getSource(0));
      }
      return nullptr;
    }
    case Kind::kAdd:
    case Kind::kAddw:
    case Kind::kXor:
    case Kind::kOr: {
      if (inst->getDestination() == targetSymbol) {
        if (inst->getSource(0) == zero) {
          return dynamic_cast<SymRegister *>(inst->getSource(1));
        }
        if (inst->getSource(1) == zero) {
          return dynamic_cast<SymRegister *>(inst->getSource(0));
        }
      }
      return nullptr;
    }
    case Kind::kAndi: {
      auto iInst = dynamic_cast<IInst *>(inst);
      if (iInst->getDestination() == targetSymbol && iInst->getImm() == (2 << 13) - 1) {
        return dynamic_cast<SymRegister *>(iInst->getSource(0));
      }
      return nullptr;
    }
    case Kind::kFsgnj_s:
    case Kind::kFmax_s:
    case Kind::kFmin_s: {
      if (inst->getDestination() == targetSymbol && inst->getSource(0) == inst->getSource(1)) {
        return dynamic_cast<SymRegister *>(inst->getSource(0));
      }
      return nullptr;
    }
    default:
      return nullptr;
  }

  return nullptr;
}
void ExtendedLinearScan::allocateRegForInterval(Interval *interval, unsigned firstPosition) {
  auto symbol = interval->symbol;
  if (!isSpilled[symbol]) {
    auto intSymbol = dynamic_cast<IntSymRegister *>(interval->symbol);
    auto floatSymbol = dynamic_cast<FloatSymRegister *>(interval->symbol);
    if (intSymbol != nullptr) {
      auto prevAlloc = prevAssign[intSymbol];
      IntSymRegister *moveSource;
      if (std::find(intAvail.begin(), intAvail.end(), prevAlloc) != intAvail.end()) {
        intAvail.remove(prevAlloc);
        interval->physicRegister = prevAlloc;
      } else {
        moveSource = dynamic_cast<IntSymRegister *>(getMoveSource(firstPosition, intSymbol));
        if (moveSource != nullptr &&
            std::find(intAvail.begin(), intAvail.end(), prevAssign[moveSource]) != intAvail.end()) {
          prevAlloc = prevAssign[moveSource];
          intAvail.remove(prevAlloc);
          interval->physicRegister = prevAlloc;
          prevAssign[intSymbol] = prevAlloc;
        } else {
          auto alloca = intAvail.front();
          interval->physicRegister = alloca;
          intAvail.pop_front();
          prevAssign[intSymbol] = alloca;
        }
      }
    } else {
      auto prevAlloc = prevAssign[floatSymbol];
      FloatSymRegister *moveSource;
      if (std::find(floatAvail.begin(), floatAvail.end(), prevAlloc) != floatAvail.end()) {
        floatAvail.remove(prevAlloc);
        interval->physicRegister = prevAlloc;
      } else {
        moveSource = dynamic_cast<FloatSymRegister *>(getMoveSource(firstPosition, floatSymbol));
        if (moveSource != nullptr &&
            std::find(floatAvail.begin(), floatAvail.end(), prevAssign[moveSource]) != floatAvail.end()) {
          prevAlloc = prevAssign[moveSource];
          floatAvail.remove(prevAlloc);
          interval->physicRegister = prevAlloc;
          prevAssign[floatSymbol] = prevAlloc;
        } else {
          auto alloca = floatAvail.front();
          interval->physicRegister = alloca;
          floatAvail.pop_front();
          prevAssign[floatSymbol] = alloca;
        }
      }
    }
  }
}
void ExtendedLinearScan::releaseRegForInterval(Interval *interval) {
  auto symbol = interval->symbol;
  if (!isSpilled[symbol]) {
    if (dynamic_cast<IntSymRegister *>(symbol) != nullptr) {
      intAvail.push_back(interval->physicRegister);
    } else {
      floatAvail.push_back(interval->physicRegister);
    }
  }
}
void ExtendedLinearScan::initInstructionList(Function *function) {
  unsigned begin = 0;
  for (const auto &block : function->getBlocks()) {
    blockList.emplace(block.get(), begin);
    for (const auto &inst : block->getInstructions()) {
      instructionList.emplace(begin, inst.get());
      begin++;
    }
  }
}
void ExtendedLinearScan::initBasicElements(
    Function *function, const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable) {
  for (const auto &block : function->getBlocks()) {
    auto begin = blockList[block.get()];

    std::map<SymRegister *, std::pair<unsigned, unsigned>> tmp;
    const auto &instructions = block->getInstructions();

    auto activeSet = activeTable.at(block.get()).at(0);
    for (const auto &active : activeSet) {
      auto leftPoint = findOrEmplaceEndPoints(2 * begin, block.get(), activeTable);
      auto rightPoint = leftPoint;
      auto interval = new Interval(leftPoint, rightPoint, active);
      leftPoint->intervals.emplace_back(interval);
      symbolIntervalMap[active].emplace_back(interval);
    }
    auto instructionIter = std::next(instructions.begin());
    for (unsigned i = 1; i < instructions.size(); i++) {
      auto activeSet = activeTable.at(block.get()).at(i);
      auto defined = dynamic_cast<SymRegister *>(instructionIter->get()->getDestination());
      for (const auto &active : activeSet) {
        if (symbolIntervalMap.find(active) == symbolIntervalMap.end()) {
          symbolIntervalMap.emplace(active, std::list<std::unique_ptr<Interval>>{});
        }
        auto tmpIter = tmp.find(active);
        if (tmpIter == tmp.end()) {
          tmp.emplace(active, std::pair<unsigned, unsigned>{begin + i - 1, begin + i});
        } else if (tmpIter->second.second + 1 == begin + i) {
          tmpIter->second.second++;
          if (defined == active) {
            auto leftPoint = findOrEmplaceEndPoints(2 * tmpIter->second.first + 1, block.get(), activeTable);
            auto rightPoint = findOrEmplaceEndPoints(2 * tmpIter->second.second, block.get(), activeTable);
            auto interval = new Interval(leftPoint, rightPoint, active);
            leftPoint->intervals.emplace_back(interval);
            rightPoint->intervals.emplace_back(interval);
            symbolIntervalMap[active].emplace_back(interval);
            tmp.erase(active);
          }
        } else {
          auto leftPoint = findOrEmplaceEndPoints(2 * tmpIter->second.first + 1, block.get(), activeTable);
          auto rightPoint = findOrEmplaceEndPoints(2 * tmpIter->second.second, block.get(), activeTable);
          auto interval = new Interval(leftPoint, rightPoint, active);
          leftPoint->intervals.emplace_back(interval);
          rightPoint->intervals.emplace_back(interval);
          symbolIntervalMap[active].emplace_back(interval);
          tmpIter->second.first = begin + i - 1;
          tmpIter->second.second = begin + i;
        }
      }
      instructionIter++;
    }

    for (const auto &tmpItem : tmp) {
      auto leftPoint = findOrEmplaceEndPoints(2 * tmpItem.second.first + 1, block.get(), activeTable);
      auto rightPoint = findOrEmplaceEndPoints(2 * tmpItem.second.second, block.get(), activeTable);
      auto interval = new Interval(leftPoint, rightPoint, tmpItem.first);
      leftPoint->intervals.emplace_back(interval);
      rightPoint->intervals.emplace_back(interval);
      symbolIntervalMap[tmpItem.first].emplace_back(interval);
    }

    activeSet = activeTable.at(block.get()).back();
    for (const auto &active : activeSet) {
      auto rightPoint =
          findOrEmplaceEndPoints(2 * (begin + block->getNumInstructions() - 1) + 1, block.get(), activeTable);
      auto leftPoint = rightPoint;
      auto interval = new Interval(leftPoint, rightPoint, active);
      rightPoint->intervals.emplace_back(interval);
      symbolIntervalMap[active].emplace_back(interval);
    }
  }
}
void ExtendedLinearScan::initActiveSymbolEndPoint() {
  for (const auto &intervalItem : symbolIntervalMap) {
    activeSymbolEndPointMap.emplace(intervalItem.first, std::list<EndPoint *>{});
  }
  for (const auto &endPointItem : positonEndPointMap) {
    for (const auto &active : endPointItem.second->intActiveSet) {
      activeSymbolEndPointMap[active].emplace_back(endPointItem.second.get());
    }
    for (const auto &active : endPointItem.second->floatActiveSet) {
      activeSymbolEndPointMap[active].emplace_back(endPointItem.second.get());
    }
  }
}
void ExtendedLinearScan::initFreq(Function *function) {
  for (const auto &block : function->getBlocks()) {
    auto begin = blockList[block.get()];
    for (unsigned i = 0; i < block->getNumInstructions(); i++) {
      freq[begin + i] = 1;
    }
  }
}
void ExtendedLinearScan::initSpill(Function *function) {
  for (const auto &intervalItem : symbolIntervalMap) {
    isSpilled.emplace(intervalItem.first, false);
    totalSpillCost.emplace(intervalItem.first, 0);
  }

  for (const auto &block : function->getBlocks()) {
    auto pos = blockList[block.get()];
    for (const auto &inst : block->getInstructions()) {
      auto define = dynamic_cast<SymRegister *>(inst->getDestination());
      if (totalSpillCost.find(define) != totalSpillCost.end()) {
        totalSpillCost[define] += freq[pos];
      }
      auto &useSet = inst->getSources();
      for (const auto &use : useSet) {
        auto symbolUse = dynamic_cast<SymRegister *>(use);
        if (totalSpillCost.find(symbolUse) != totalSpillCost.end()) {
          totalSpillCost[symbolUse] += freq[pos];
        }
      }
      pos++;
    }
  }

  for (const auto &endPointItem : positonEndPointMap) {
    endPointItem.second->freq = freq[endPointItem.first / 2];
  }
}
void ExtendedLinearScan::initPrevAssign() {
  for (const auto &intervalItem : symbolIntervalMap) {
    prevAssign.emplace(intervalItem.first, 0);
  }
}
void ExtendedLinearScan::spillIdentification() {
  std::list<EndPoint *> intSpillEndPoints;
  std::list<EndPoint *> floatSpillEndPoints;
  for (const auto &endPointItem : positonEndPointMap) {
    if (endPointItem.second->intActiveSet.size() > numIntRegisters) {
      intSpillEndPoints.emplace_back(endPointItem.second.get());
    }
    if (endPointItem.second->floatActiveSet.size() > numFloatRegisters) {
      floatSpillEndPoints.emplace_back(endPointItem.second.get());
    }
  }

  auto freqComp = [](const EndPoint *endPoint1, const EndPoint *endPoint2) -> bool {
    return endPoint1->freq > endPoint2->freq;
  };
  intSpillEndPoints.sort(freqComp);
  floatSpillEndPoints.sort(freqComp);

  auto costIntComp = [&](IntSymRegister *intSymbol1, IntSymRegister *intSymbol2) -> bool {
    return totalSpillCost[intSymbol1] < totalSpillCost[intSymbol2];
  };
  while (!intSpillEndPoints.empty()) {
    auto intEndPoint = intSpillEndPoints.front();
    while (intEndPoint->intActiveSet.size() > numIntRegisters) {
      auto intSymbol =
          *(std::min_element(intEndPoint->intActiveSet.begin(), intEndPoint->intActiveSet.end(), costIntComp));
      isSpilled[intSymbol] = true;
      intSpillStack.push(intSymbol);

      for (const auto &endPoint : activeSymbolEndPointMap[intSymbol]) {
        endPoint->intActiveSet.erase(intSymbol);
      }
    }
    intSpillEndPoints.pop_front();
  }

  auto costFloatComp = [&](FloatSymRegister *floatSymbol1, FloatSymRegister *floatSymbol2) -> bool {
    return totalSpillCost[floatSymbol1] < totalSpillCost[floatSymbol2];
  };
  while (!floatSpillEndPoints.empty()) {
    auto floatEndPoint = floatSpillEndPoints.front();
    while (floatEndPoint->floatActiveSet.size() > numFloatRegisters) {
      auto floatSymbol = *(
          std::min_element(floatEndPoint->floatActiveSet.begin(), floatEndPoint->floatActiveSet.end(), costFloatComp));
      isSpilled[floatSymbol] = true;
      floatSpillStack.push(floatSymbol);

      for (const auto &endPoint : activeSymbolEndPointMap[floatSymbol]) {
        endPoint->floatActiveSet.erase(floatSymbol);
      }
    }
    floatSpillEndPoints.pop_front();
  }
}
void ExtendedLinearScan::spillResurrection() {
  while (!intSpillStack.empty()) {
    auto intSpillSymbol = intSpillStack.top();
    intSpillStack.pop();
    bool canResurrection = true;
    for (const auto &endPoint : activeSymbolEndPointMap[intSpillSymbol]) {
      if (endPoint->intActiveSet.size() == numIntRegisters) {
        canResurrection = false;
        break;
      }
    }

    if (canResurrection) {
      isSpilled[intSpillSymbol] = false;
      for (const auto &endPoint : activeSymbolEndPointMap[intSpillSymbol]) {
        endPoint->intActiveSet.insert(intSpillSymbol);
      }
    }
  }

  while (!floatSpillStack.empty()) {
    auto floatSpillSymbol = floatSpillStack.top();
    floatSpillStack.pop();
    bool canResurrection = true;
    for (const auto &endPoint : activeSymbolEndPointMap[floatSpillSymbol]) {
      if (endPoint->floatActiveSet.size() == numFloatRegisters) {
        canResurrection = false;
        break;
      }
    }

    if (canResurrection) {
      isSpilled[floatSpillSymbol] = false;
      for (const auto &endPoint : activeSymbolEndPointMap[floatSpillSymbol]) {
        endPoint->floatActiveSet.insert(floatSpillSymbol);
      }
    }
  }
}
void ExtendedLinearScan::registerAllocate() {
  for (const auto &endPointItem : positonEndPointMap) {
    if (endPointItem.first % 2 == 0) {
      for (const auto &interval : endPointItem.second->intervals) {
        if (interval->leftEndPoint == interval->rightEndPoint) {
          allocateRegForInterval(interval, endPointItem.first / 2);
        }
      }
      for (const auto &interval : endPointItem.second->intervals) {
        releaseRegForInterval(interval);
      }
    } else {
      std::list<Interval *> specialIntervals;
      for (const auto &interval : endPointItem.second->intervals) {
        allocateRegForInterval(interval, endPointItem.first / 2);
        if (interval->leftEndPoint == interval->rightEndPoint) {
          specialIntervals.emplace_back(interval);
        }
      }
      for (const auto &interval : specialIntervals) {
        releaseRegForInterval(interval);
      }
    }
  }
}
// 有待拓展
void ExtendedLinearScan::registerMap() {
  auto intIter = intPhyRegisterPool.begin();
  for (unsigned int i = 0; i < numIntRegisters; i++) {
    intRegisterMap[i] = *intIter;
    intIter++;
  }

  auto floatIter = floatPhyRegisterPool.begin();
  for (unsigned int i = 0; i < numFloatRegisters; i++) {
    floatRegisterMap[i] = *floatIter;
    floatIter++;
  }
}
void ExtendedLinearScan::recordResult(Function *function) {
  functionSymbolMap.emplace(function, std::list<SymRegister *>{});
  for (const auto &intervalItem : symbolIntervalMap) {
    auto symbol = intervalItem.first;
    totalSymbolIntervalMap.emplace(symbol, std::list<std::unique_ptr<ResultInterval>>{});
    functionSymbolMap[function].emplace_back(symbol);
    if (dynamic_cast<IntSymRegister *>(symbol) != nullptr) {
      for (const auto &interval : intervalItem.second) {
        auto resultInterval = new ResultInterval;
        resultInterval->left = interval->leftEndPoint->position;
        resultInterval->right = interval->rightEndPoint->position;
        resultInterval->physicRegister = intRegisterMap[interval->physicRegister];
        totalSymbolIntervalMap[symbol].emplace_back(resultInterval);
      }
    } else {
      for (const auto &interval : intervalItem.second) {
        auto resultInterval = new ResultInterval;
        resultInterval->left = interval->leftEndPoint->position;
        resultInterval->right = interval->rightEndPoint->position;
        resultInterval->physicRegister = floatRegisterMap[interval->physicRegister];
        totalSymbolIntervalMap[symbol].emplace_back(resultInterval);
      }
    }
  }
}
}  // namespace riscv
