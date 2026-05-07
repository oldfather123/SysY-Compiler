#pragma once

#include <list>
#include <memory>
#include <set>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

/**
 * @file FuncInline.h
 *
 * @brief 定义函数内联的头文件
 */
namespace sysy {
/**
 * @brief 函数内联
 *
 * @note 时间和编译时间有限，先暂时设计两种策略，如下：
 * 策略一：main函数之外的其他函数内联一层，然后main函数再内联一层；
 * 策略二：将函数调用图进行拓扑排序，然后按照拓扑排序的逆序进行内联；
 * 两种策略都采用了经验性的阈值与条件；
 * 更优的optimal function inline 等后续还有时间再参考 ASPLOS'2022 的 Best Paper
 */
class FuncInline {
 private:
  Module *pModule;      ///< 模块
  IRBuilder *pBuilder;  ///< IR构建器
  int threshold;        ///< 内联阈值
  int below;            ///< 内联下界

  unsigned cplabelIndex = 0;   ///< 用于生成新的label下标
  unsigned cpallocaIndex = 0;  ///< 用于生成新的alloca下标
  unsigned cptmpIndex = 0;     ///< 用于生成新的临时变量下标

  std::list<std::unique_ptr<Function>> record;  ///< 记录内联的函数，用作RAII

 public:
  FuncInline(Module *pMoudle, IRBuilder *pBuilder, int threshold = 128, int below = 16)
      : pModule(pMoudle), pBuilder(pBuilder), threshold(threshold), below(below) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行函数内联

  auto myinline(Function *func) -> void;  ///< myinline

  // private helper function.
  auto getBlockListBefore(Function *func) -> std::list<BasicBlock *>;  ///< 获取运行inline之前函数的所有块
  auto usedelete(Instruction *instr) -> void;                          ///< 删除指令相关的value-use-user关系
  auto getReturnBlocks(Function *func) -> std::set<BasicBlock *>;      ///< 获取函数的返回块

  auto getcpLabelIndex() -> unsigned {
    cplabelIndex += 1;
    return cplabelIndex - 1;
  }  ///< 获取新的label下标
  auto getcpAllocaIndex() -> unsigned {
    cpallocaIndex += 1;
    return cpallocaIndex - 1;
  }  ///< 获取新的alloca下标
  auto getcpTmpIndex() -> unsigned {
    cptmpIndex += 1;
    return cptmpIndex - 1;
  }  ///< 获取新的临时变量下标

  auto isSubArray(Value *argI) -> bool;  ///< 判断是否是子数组
};
}  // namespace sysy
