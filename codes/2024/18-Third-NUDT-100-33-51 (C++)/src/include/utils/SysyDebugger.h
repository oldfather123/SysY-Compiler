#pragma once

#include <string>
#include "../backend/BackEndActiveAnalysis.h"
#include "../backend/InstructionSchedule.h"
#include "../backend/RegisterAlloc.h"
#include "../backend/Riscv.h"
#include "../frontend/IR.h"
#include "../midend/DataFlowAnalysis.h"

namespace sysy {
class SysyDebugger {
 public:
  // 打印控制图前驱和后继
  static auto printCFG(Module *module) -> void;

  // 打印支配前驱和支配后继
  static auto printDomTree(Module *module) -> void;

  // 打印value2DefBlocks和value2UseBlocks
  static auto printValue2Blocks(Module *module) -> void;

  // 打印指令的相关信息
  static auto printIR4Info(Module *module) -> void;

  // 打印单条IR
  static auto printSingleIR(Instruction *inst) -> void;

  // 打印单条IR被谁使用
  static auto printSingleIRUsedBy(Instruction *inst) -> void;

  // 用于IR打印name和operands的辅助函数
  static auto getOpName(Value *operand) -> std::string;

  // 打印外部函数
  static auto printExternalFunc(Module *module) -> void;

  // 打印递归函数
  static auto printRecursiveFunc(Module *module) -> void;

  // 打印函数的代码体积
  static auto printEachFuncCodeSize(Module *module) -> void;

  // 打印函数调用图
  static auto printCallGraph(Module *module) -> void;

  // 打印函数的属性
  static auto printFuncAttributes(Module *module) -> void;

  // 打印函数的全局变量写集
  static auto printGlobalWirttenSet(Module *module) -> void;

  static auto printOriFuncIRInfoAndCLoneFuncIRInfo(Module *module) -> void;

  // 用于打印Loop关系
  static auto printLoopInfo(Module *module) -> void;

  // 打印数据流分析结果，比如活跃变量分析结果等
  static auto printDataFlow(DataFlowManager &dataFlowManager, Module *module) -> void;

  // 打印后端活跃变量分析结果
  static auto printBackEndActiveAnalysis(riscv::ActiveVarAnalysis &activeAnalysis, riscv::Module *module) -> void;

  // 打印扩展线性扫描器的基本块列
  static auto printBackEndELSBasicBlocks(const riscv::ExtendedLinearScan &extendedLinearScan, riscv::Module *pModule)
      -> void;

  // 打印后端寄存器分配结果
  static auto printBackEndELSMapResult(const riscv::ExtendedLinearScan &extendedLinearScan) -> void;

  // 打印指令调度的结果
  static auto printScheduledInstructions(const riscv::InstructionSchedule &instructionSchedule) -> void;

  static auto printBackEndDependGraph(const riscv::InstructionSchedule &instructionSchedule) -> void;
  // 打印后端的Module, function, basicblock
  static auto printBackEndBasicblockInfo(riscv::Module *pModule) -> void;
  //打印指令调度的结果
};
}  // namespace sysy
