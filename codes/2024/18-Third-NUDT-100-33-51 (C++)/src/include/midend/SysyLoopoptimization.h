/**
 * @file: SysyLoopoptimization.h
 * @brief  循环不变量外提
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-02
 *
 */
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

namespace sysy {

/**
 * @brief  循环不变量外提，用来对循环进行不变量外提
 */
class SysyLoopoptimization {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  SysyLoopoptimization(Module *pModuleIn, IRBuilder *pBuilderIn) : pModule(pModuleIn), pBuilder(pBuilderIn){};

  auto SysyrunInvariant() -> void;  ///< 循环不变量外提，对所有循环进行不变量外提

  auto SysyInvariantBasicLoop(Loop *toploop) -> void;  ///< 对单个循环做不变量外提，供SysyrunInvariant函数使用

  auto SysyPrestore(Loop *toploop) -> void;
};
}  // namespace sysy