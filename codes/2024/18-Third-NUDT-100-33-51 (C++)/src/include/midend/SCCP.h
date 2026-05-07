#pragma once

#include "../frontend/IR.h"

/**
 * @file SCCP.h
 *
 * @brief 定义SCCP的头文件
 */

namespace sysy {
/**
 * @brief 稀疏条件常量传播
 *
 */
class SCCP {
 private:
  Module *pModule;  ///< 模块

 public:
  explicit SCCP(Module *pMoudle) : pModule(pMoudle) {}  ///< 初始化函数

  auto run() -> void;  ///< 稀疏条件常量传播

  // private helper function.
  auto isGlobal(Value *val) -> bool;           ///< 判断是否是全局变量
  auto isArr(Value *val) -> bool;              ///< 判断是否是数组
  auto usedelete(Instruction *instr) -> void;  ///< 删除指令相关的value-use-user关系
};
}  // namespace sysy
