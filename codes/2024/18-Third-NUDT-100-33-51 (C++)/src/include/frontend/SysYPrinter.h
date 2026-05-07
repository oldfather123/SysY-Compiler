#pragma once

#include <string>
#include "../frontend/IR.h"

namespace sysy {
/**
 * \defgroup printer IRPrinter
 *
 * @{
 */

/**
 * @brief IR打印器
 */
class SysYPrinter {
 private:
  Module *pModule;  ///< 要打印的模块

 public:
  explicit SysYPrinter(Module *pModule) : pModule(pModule) {}

 public:
  void printIR(const std::string &fileName = "");  ///< 打印IR
  void printGlobal();                              ///< 打印全局变量
  static void printFunction(Function *function);   ///< 打印函数
  static void printInst(Instruction *pInst);       ///< 打印指令

 public:
  static auto getOperandName(Value *operand) -> std::string;  ///< 获取操作数名字
};
/**
 *@}
 */
}  // namespace sysy
