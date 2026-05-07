#pragma once

#include <unordered_set>
#include "../frontend/IR.h"

/**
 * @file CFA.h
 *
 * @brief 定义CFA的头文件
 */

namespace sysy {
/**
 * @brief 控制流分析
 *
 */
class CFA {
 private:
  Module *pModule;  ///< 模块

 public:
  explicit CFA(Module *pMoudle) : pModule(pMoudle) {}  ///< 初始化函数

  auto computeDom() -> void;  ///< 计算必经结点

  auto computeDomTree() -> void;  ///< 隐式构造支配树

  auto computeDF(BasicBlock *block) -> std::unordered_set<BasicBlock *>;  ///< 计算单个块的支配边界，递归实现
  auto computeDFAll() -> void;                                            ///< 计算所有块的支配边界

  auto run() -> void;  ///< 运行控制流分析

 private:
  auto intersect(std::unordered_set<BasicBlock *> &dom, const std::unordered_set<BasicBlock *> &other)
      -> void;  ///< 交集，dom = dom & other
};
}  // namespace sysy
