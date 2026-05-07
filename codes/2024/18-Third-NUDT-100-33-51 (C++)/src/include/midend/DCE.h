#pragma once

#include "../frontend/IR.h"

/**
 * @file DCE.h
 *
 * @brief 定义DCE的头文件
 */
namespace sysy {
/**
 * @brief 死代码删除
 *
 */
class DCE {
 private:
  Module *pModule;  ///< 模块

 public:
  explicit DCE(Module *pMoudle) : pModule(pMoudle) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行死代码删除

  // private helper function.
  auto isGlobal(Value *val) -> bool;           ///< 判断是否是全局变量
  auto isArr(Value *val) -> bool;              ///< 判断是否是数组
  auto usedelete(Instruction *instr) -> void;  ///< 删除指令相关的value-use-user关系
};
}  // namespace sysy
