/**
 * @file: SysYLoopAnalysis.h
 * @brief  循环分析.h
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#pragma once

#include <set>
#include <stack>
#include <vector>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"
namespace sysy {
/**
 * @brief  循环分析类，用于循环分析
 */
class SysYLoopAnalysis {
 private:
  Module *pModule;
  IRBuilder *pBuilder;
  std::vector<Loop *> looptodelete;

 public:
  SysYLoopAnalysis(Module *pModule, IRBuilder *pBuilder) : pModule(pModule), pBuilder(pBuilder) {}

  auto Loopclear() -> void;  ///< 清除之前的循环分析结果

  auto LoopAnalysize() -> void;  ///< 循环分析

  auto LoopfindSimpleLoopVariables() -> void;  ///< 循环分析，找到简单循环的变量

  auto DFSBasicBlock2VisitOrder(BasicBlock *entry, std::set<BasicBlock *> &visited,
                                std::vector<BasicBlock *> &visitOrder) -> void;  ///< 深度优先遍历，得到逆拓扑排序

  auto discoverAndMapSubloop(Loop *loop, Function *function, std::stack<BasicBlock *> &BackedgeTo);  ///< 找subloop

  auto DFSBasicBlock2GetsubLoop(BasicBlock *entry, std::set<BasicBlock *> &visited) -> void;  ///< 深度优先找subloop

  auto clean() -> void;  ///< no used

  auto getPhi(Instruction *instr, Loop *loop) -> AllocaInst *;  ///< 获得phi

  auto ScalarEvolution(Loop *loop) -> void;  ///< no use

  auto runBfSSA() -> void;  ///< SSA前进行循环分析，为其他优化做分析

  auto runAfSSA() -> void;  ///< SSA后进行循环分析，为循环优化做分析

  auto LoopfindPhiBeginAndStep() -> void;  ///< 寻找循环开始和步长

  auto LoopfindGlobalchange() -> void;  ///< 寻找循环内改变的globalvalue

  auto LoopParallelCheck() -> void;
};
}  // namespace sysy
