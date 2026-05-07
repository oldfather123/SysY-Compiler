#pragma once

#include <list>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

/**
 * @file SysySSA.h
 *
 * @brief 定义mem2reg的头文件
 */

namespace sysy {
/**
 * @brief 静态单一赋值 mem2reg
 *
 */
class Mem2Reg {
 private:
  Module *pModule;      ///< 模块
  IRBuilder *pBuilder;  ///< IR构建器

 public:
  Mem2Reg(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder) {}  ///< 初始化函数

  auto run() -> void;  ///< mem2reg

 private:
  auto computeIterDf(const std::unordered_set<BasicBlock *> &blocks)
      -> std::unordered_set<BasicBlock *>;  ///< 计算定义块集合的迭代支配边界

  auto computeValue2Blocks() -> void;  ///< 计算value2block的映射(不包括数组和global)

  // llvm memtoreg类预优化
  auto preOptimize1() -> void;  ///< llvm memtoreg预优化1: 删除不含load的alloc和store
  auto preOptimize2() -> void;  ///< llvm memtoreg预优化2: 针对某个变量的Defblocks只有一个块的情况
  auto preOptimize3() -> void;  ///< llvm memtoreg预优化3: 针对某个变量的所有读写都在同一个块中的情况

  auto insertPhi() -> void;  ///< 为所有变量的迭代支配边界插入phi结点

  auto rename(BasicBlock *block, std::unordered_map<Value *, int> &count,
              std::unordered_map<Value *, std::stack<Instruction *>> &stacks) -> void;  ///< 单个块的重命名
  auto renameAll() -> void;                                                             ///< 重命名所有块

  // private helper function.
 private:
  auto getPredIndex(BasicBlock *n, BasicBlock *s) -> int;  ///< 获取前驱索引
  auto cascade(Instruction *instr, bool &changed, Function *func, BasicBlock *block,
               std::list<std::unique_ptr<Instruction>> &instrs) -> void;  ///< 消除级联关系
  auto isGlobal(Value *val) -> bool;                                      ///< 判断是否是全局变量
  auto isArr(Value *val) -> bool;                                         ///< 判断是否是数组
  auto usedelete(Instruction *instr) -> void;                             ///< 删除指令相关的value-use-user关系
};
}  // namespace sysy
