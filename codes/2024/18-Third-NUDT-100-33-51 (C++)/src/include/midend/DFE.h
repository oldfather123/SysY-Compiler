#pragma once

#include "../frontend/IR.h"

/**
 * @file DFE.h
 *
 * @brief 定义DFE的头文件
 */
namespace sysy {
/**
 * @brief DFE (Dead Function Elimination) : call无用消除，死函数消除
 */
class DFE {
 private:
  Module *pModule;  ///< 模块

 public:
  explicit DFE(Module *pMoudle) : pModule(pMoudle) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行DFE

 private:
  auto usedelete(Instruction *instr) -> void;        ///< 删除指令相关的value-use-user关系
  auto isUsedOnlyByOneFunc(Function *func) -> bool;  ///< 判断函数是否只被一个函数调用
};
}  // namespace sysy
