#pragma once

#include <set>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

/**
 * @file TailCallElimination.h
 *
 * @brief 定义尾调用消除的头文件
 */
namespace sysy {
/**
 * @brief 尾调用消除
 *
 */
class TailCallElimination {
 private:
  Module *pModule;      ///< 模块
  IRBuilder *pBuilder;  ///< IR构建器

 public:
  TailCallElimination(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行尾调用消除

 private:
  auto getReturnBlocks(Function *func) -> std::set<BasicBlock *>;  ///< 获取函数的返回块
  auto usedelete(Instruction *instr) -> void;                      ///< 删除指令相关的value-use-user关系
  auto hasArray(const std::vector<AllocaInst *> &argVec) -> bool;  ///< 判断是否含有数组
  auto isArr(Value *val) -> bool;                                  ///< 判断是否是数组
};
}  // namespace sysy
