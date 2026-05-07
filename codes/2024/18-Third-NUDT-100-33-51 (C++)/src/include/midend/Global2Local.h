#pragma once

#include <map>
#include <set>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

/**
 * @file Global2Local.h
 *
 * @brief 定义Global2Local的头文件
 */

namespace sysy {
/**
 * @brief Global2Local
 */
class Global2Local {
 private:
  Module *pModule;      ///< 模块
  IRBuilder *pBuilder;  ///< IR构建器

  std::map<GlobalValue *, std::set<Function *>> global2func;  ///< 全局变量对应的函数

 public:
  Global2Local(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder) {}  ///< 初始化函数

  auto run() -> void;  ///< 运行G2L

 private:
  auto calculateg2f() -> void;  ///< 计算全局变量对应的函数

 private:
  auto usedelete(Instruction *instr) -> void;              ///< 删除指令相关的value-use-user关系
  auto GisUsedByGetSubArray(GlobalValue *global) -> bool;  ///< 判断全局变量是否被GetSubArray使用
};
}  // namespace sysy
