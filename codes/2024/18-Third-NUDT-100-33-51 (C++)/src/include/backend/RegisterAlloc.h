#pragma once

#include <cassert>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <utility>
#include <vector>
#include "../backend/Riscv.h"

/**
 * @file RegisterAlloc.h
 *
 * 定义了寄存器分配的头文件
 */

// 规定整型寄存器使用t0, t1， 浮点型寄存器使用ft0, ft1, ft2作为溢出寄存器(可能造成寄存器浪费)

namespace riscv {

class ExtendedLinearScan {
 public:
  static const std::list<IntPhyRegister *> intPhyRegisterPool;
  static const std::list<FloatPhyRegister *> floatPhyRegisterPool;

 private:
  struct Interval;
  //! 区间端点
  using EndPoint = struct EndPoint {
    unsigned position;                            ///< 端点位置
    unsigned freq;                                ///< 端点执行频率
    std::set<IntSymRegister *> intActiveSet;      ///< 活跃整型符号寄存器集合
    std::set<FloatSymRegister *> floatActiveSet;  ///< 活跃浮点型符号寄存器集合
    std::list<Interval *> intervals;              ///< 与该端点相关联的区间列表

    template <typename ContainerT>
    void setActiveSet(const ContainerT &container) {
      for (const auto &active : container) {
        auto intSymbol = dynamic_cast<IntSymRegister *>(active);
        auto floatSymbol = dynamic_cast<FloatSymRegister *>(active);
        if (intSymbol != nullptr) {
          intActiveSet.insert(intSymbol);
        } else if (floatSymbol != nullptr) {
          floatActiveSet.insert(floatSymbol);
        } else {
          assert(false);
        }
      }
    }
  };
  //! 区间
  using Interval = struct Interval {
    SymRegister *symbol;      ///< 所属的符号寄存器
    EndPoint *leftEndPoint;   ///< 左端点
    EndPoint *rightEndPoint;  ///< 右端点

    unsigned physicRegister{};  ///< 被分配的物理寄存器

    Interval(EndPoint *leftEndPoint, EndPoint *rightEndPoint, SymRegister *symbol)
        : symbol(symbol), leftEndPoint(leftEndPoint), rightEndPoint(rightEndPoint) {}
  };

 public:
  //! 结果中的区间表示
  using ResultInterval = struct ResultInterval {
    unsigned left;
    unsigned right;
    PhyRegister *physicRegister;
  };

 private:
  unsigned numIntRegisters;    ///< 整型寄存器可用数量
  unsigned numFloatRegisters;  ///< 浮点型寄存器可用数量

 private:
  // 临时区
  std::map<unsigned, Instruction *> instructionList;                                ///< 线性化的指令列表
  std::map<SymRegister *, std::list<std::unique_ptr<Interval>>> symbolIntervalMap;  ///< 符号寄存器-区间对应
  std::map<unsigned, std::unique_ptr<EndPoint>>
      positonEndPointMap;  ///< 位置-端点对应(位置为偶数表示读区，奇数则为写区)
  std::map<SymRegister *, std::list<EndPoint *>> activeSymbolEndPointMap;  ///< 活跃符号寄存器-端点对应

  std::map<SymRegister *, unsigned> totalSpillCost;  ///< 总溢出代价
  std::map<unsigned, unsigned> freq;                 ///< 每条指令的执行频率

  std::stack<IntSymRegister *> intSpillStack;      ///< 整型符号寄存器溢出栈
  std::stack<FloatSymRegister *> floatSpillStack;  ///< 浮点型寄存器溢出栈

  std::list<unsigned> intAvail;                  ///< 当前可用的整型寄存器
  std::list<unsigned> floatAvail;                ///< 当前可用的浮点型寄存器
  std::map<SymRegister *, unsigned> prevAssign;  ///< 最近一次的的分配结果

  std::map<unsigned, IntPhyRegister *> intRegisterMap;      ///< 整型寄存器映射
  std::map<unsigned, FloatPhyRegister *> floatRegisterMap;  ///< 浮点型寄存器映射

  // 结果区
  std::map<SymRegister *, std::list<std::unique_ptr<ResultInterval>>> totalSymbolIntervalMap;  ///< 符号寄存器-区间对应
  std::map<BasicBlock *, unsigned> blockList;                        ///< 线性化的基本块列表
  std::map<SymRegister *, bool> isSpilled;                           ///< 符号寄存器是否溢出
  std::map<Function *, std::list<SymRegister *>> functionSymbolMap;  ///< 函数-符号寄存器对应

 public:
  ExtendedLinearScan(unsigned numInt, unsigned numFloat) : numIntRegisters(numInt), numFloatRegisters(numFloat) {
    assert(numInt <= intPhyRegisterPool.size());
    assert(numFloat <= floatPhyRegisterPool.size());
    for (unsigned i = 0; i < numIntRegisters; i++) {
      intAvail.push_back(i);
    }
    for (unsigned i = 0; i < numFloatRegisters; i++) {
      floatAvail.push_back(i);
    }
  }

 private:
  auto findOrEmplaceEndPoints(unsigned inputPosition, BasicBlock *block,
                              const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable)
      -> EndPoint *;
  auto getMoveSource(unsigned position, SymRegister *targetSymbol) const -> SymRegister *;
  void allocateRegForInterval(Interval *interval, unsigned firstPosition);
  void releaseRegForInterval(Interval *interval);

 private:
  // 以函数为单位
  void initInstructionList(Function *function);  ///< 初始化编号的指令列表
  void initBasicElements(
      Function *function,
      const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable);  ///< 初始化端点和区间
  void initFreq(Function *function);
  void initSpill(Function *function);
  void initActiveSymbolEndPoint();
  void initPrevAssign();

 private:
  void spillIdentification();
  void spillResurrection();
  void registerAllocate();

 private:
  void init(Function *function, const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable) {
    initInstructionList(function);
    initBasicElements(function, activeTable);
    initActiveSymbolEndPoint();
    initFreq(function);
    initSpill(function);
    initPrevAssign();
  }
  void AllocateForFunction() {
    spillIdentification();
    spillResurrection();
    registerAllocate();
  }
  void registerMap();
  void recordResult(Function *function);
  void clearTempData() {
    instructionList.clear();
    symbolIntervalMap.clear();
    positonEndPointMap.clear();
    activeSymbolEndPointMap.clear();
    totalSpillCost.clear();
    freq.clear();
    prevAssign.clear();
    intRegisterMap.clear();
    floatRegisterMap.clear();
    intAvail.clear();
    floatAvail.clear();
    for (unsigned i = 0; i < numIntRegisters; i++) {
      intAvail.push_back(i);
    }
    for (unsigned i = 0; i < numFloatRegisters; i++) {
      floatAvail.push_back(i);
    }
  }

 public:
  void AllocateForModule(Module *pModule,
                         const std::map<BasicBlock *, std::vector<std::set<SymRegister *>>> &activeTable) {
    for (const auto &funtcionItem : pModule->getFunctions()) {
      auto function = funtcionItem.second.get();
      if (!(function->isExternalFucntion())) {
        init(function, activeTable);
        AllocateForFunction();
        registerMap();
        recordResult(function);
        clearTempData();
      }
    }
  }
  void clear() {
    instructionList.clear();
    symbolIntervalMap.clear();
    positonEndPointMap.clear();
    activeSymbolEndPointMap.clear();
    totalSpillCost.clear();
    freq.clear();
    prevAssign.clear();
    intRegisterMap.clear();
    floatRegisterMap.clear();
    totalSymbolIntervalMap.clear();
    blockList.clear();
    isSpilled.clear();
    functionSymbolMap.clear();
  }

 public:
  auto getBlockList() const -> const std::map<BasicBlock *, unsigned> & { return blockList; }
  auto getAllocaResult() const -> const std::map<SymRegister *, std::list<std::unique_ptr<ResultInterval>>> & {
    return totalSymbolIntervalMap;
  }
  auto getFunctionSymbolMap() const -> const std::map<Function *, std::list<SymRegister *>> & {
    return functionSymbolMap;
  }
  auto isSymbolSpilled(SymRegister *symbol) const -> bool { return isSpilled.at(symbol); }
};
}  // namespace riscv
