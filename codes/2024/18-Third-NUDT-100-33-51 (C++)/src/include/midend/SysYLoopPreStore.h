/**
 * @file: SysYLoopPreStore.h
 * @brief  循环预存
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

namespace sysy {

/**
 * @brief  循环预存结构体，未使用
 */
class SysyLoopPreStore {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  SysyLoopPreStore(Module *pModuleIn, IRBuilder *pBuilderIn) : pModule(pModuleIn), pBuilder(pBuilderIn){};

  auto run() -> void;  ///< run

  auto runforLoop(Loop *toploop) -> void;  ///< 对某个循环执行
};
}  // namespace sysy