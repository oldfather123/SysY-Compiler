#pragma once

#include <algorithm>
#include <deque>
#include <iostream>
#include <queue>
#include <tuple>
#include "../backend/Riscv.h"

/// @brief 对常量除法进行优化的头文件

namespace riscv {
class Peephole {
 public:
  using iteratorInst = std::list<std::unique_ptr<Instruction>>::iterator;
  //   using queueiter =
 private:
  unsigned int holeSize = 2;
  BasicBlock *curBlock;
  iteratorInst curInstIter;
  std::map<Instruction *, bool> toDelete{};
  std::deque<Instruction *> slideTable{};  //< 窥孔表
  unsigned int instNum = 0;                //< 当前在表中的指令数

 public:                      //主函数
  void run(Module *pModule);  //

  void replaceDiv(BasicBlock *block, Instruction *inst, iteratorInst iter);
  //   void replaceDivu(BasicBlock *block, Instruction *inst, iterator iter);
  //   void replaceDivF(BasicBlock *block, Instruction *inst, iterator iter);
 public:  //操作私有变量的函数
  auto getTableInstNum() const -> unsigned int { return instNum; }
  auto setTableInstNum(unsigned int num) -> void { this->instNum = num; }
  auto getHoleSize() const -> unsigned int { return holeSize; }

 public:                //匹配模板
  void runAsPattern();  // 负责匹配所有可替换的模板
  void div2Mul();       // 除化乘模板
  void matchUnuseIinst();
  void delUnuseInst();
  void delLSInst();
  void delMathInst();

  auto getBeforeImm(Instruction *inst, Register *reg, int beforeNums)
      -> std::tuple<bool, int>;  // 找立即数,从iter前向搜索10条指令找li指令
  auto isDivInst(Instruction *inst) -> bool;
  auto isDivuInst(Instruction *inst) -> bool;
  auto getIterByInst(Instruction *inst) -> iteratorInst;
  //   auto isTemp();
};
}  // namespace riscv