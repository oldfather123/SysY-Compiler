#pragma once

#include <map>
#include <set>
#include "../frontend/IR.h"

/**
 * @file FuncAnalysis.h
 *
 * @brief 定义函数分析的头文件
 */

namespace sysy {
/**
 * @brief 函数分析
 */
class FuncAnalysis {
 private:
  Module *pModule;  ///< 模块

 public:
  explicit FuncAnalysis(Module *pMoudle) : pModule(pMoudle) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行函数分析

  static auto buildCallGraph(Module *inModule) -> std::map<Function *, std::set<Function *>>;  ///< 构建函数调用图

  static auto topologocalSort(const std::map<Function *, std::set<Function *>> &CG)
      -> std::vector<Function *>;  ///< 拓扑排序

  static auto getFuncCodeSize(Function *func) -> int;  ///< 获取函数代码大小

  static auto isRecursiveFunc(Function *func) -> bool;  ///< 判断是否为递归函数

  static auto hasSideEffect(Function *func) -> bool;  ///< 判断是否有副作用

  static auto isNoPureCauseMemRead(Function *func) -> bool;  ///< 判断是否有内存读导致的非纯函数

  static auto globalWirttenSet(Function *func) -> std::set<GlobalValue *>;  ///< 获取全局变量写集

 private:
  static auto setFuncAll(std::map<Function *, std::set<Function *>> &CG) -> void;  ///< 设置函数的东西

  static auto isGlobal(Value *val) -> bool;  ///< 判断是否是全局变量

  static auto isArr(Value *val) -> bool;  ///< 判断是否是数组
};
}  // namespace sysy
