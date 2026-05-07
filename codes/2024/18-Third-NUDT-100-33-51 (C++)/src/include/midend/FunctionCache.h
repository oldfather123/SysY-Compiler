#pragma once
#include "../frontend/IR.h"

/**
 * @file FunctionCache.h
 * @brief 实现FunctionCache优化的头文件
 *
 */

namespace sysy {
/**
 * @brief 使用FunctionCache优化递归函数调用
 *
 */
class FunctionCacheBuilder {
  Module *pModule;

 public:
  auto isFunctionCachedAble(Function *function) -> bool;  ///< 判断函数是否可使用FunctionCache进行优化
  void run();                                             ///< 进行FunctionCache优化

 public:
  explicit FunctionCacheBuilder(Module *pModule) : pModule(pModule){};
};
}  // namespace sysy