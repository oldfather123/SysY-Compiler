#pragma once

#include <string>
#include "../backend/Riscv.h"

/**
 * @file RiscvPrinter.h
 *
 * @brief 打印Riscv后端汇编指令的头文件
 */

namespace riscv {
/**
 * @brief riscv符号寄存器形式指令的打印器
 *
 */
class RiscvCodePrinter {
 public:
  RiscvCodePrinter() = default;

 public:
  void printRiscv(const std::string &fileName, Module *pModule);
  void printGlobal(Module *pModule);
  void printFunction(Function *function);
  static void printInst(Instruction *inst);

 public:
  static auto getOperandName(Register *reg) -> std::string;
  static auto getInstName(Instruction *inst) -> std::string;
  static void printRInst(RInst *inst);
  static void printIInst(IInst *inst);
  static void printUInst(UInst *inst);
  static void printSInst(SInst *inst);
  static void printBInst(BInst *inst);
  static void printJInst(JInst *inst);
  static void printI2FInst(I2FInst *inst);
  static void printF2IInst(F2IInst *inst);
  static void printFRInst(FRInst *inst);
  static void printFR2IInst(FR2IInst *inst);
  static void printFLongInst(FLongInst *inst);
  static void printFLInst(FLInst *inst);
  static void printFSInst(FSInst *inst);
  static void printLLaInst(LlaInst *inst);
  static void printLiInst(LiInst *inst);
  static void printSext_wInst(Sext_wInst *inst);
  static void printCallInst(CallInst *inst);
};
}  // namespace riscv
