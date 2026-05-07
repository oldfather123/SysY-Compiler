
/**
 * @file: Sysyoptimization.h
 * @brief  中端优化
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-06-30
 *
 */
#pragma once
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

namespace sysy {

/**
 * @brief  对CFG进行优化，类
 */
class SysyOptimization {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  /// @brief 构造函数
  SysyOptimization(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder) {}

  auto SysyDelAfterBr() -> void;  ///< 删除condbr/br 后的指令

  auto SysyCondbr2Br() -> void;  ///< condbr 1/0 删除

  auto SysyBlockMerge() -> void;  ///< 链合并

  auto SysyDelEmptyBlock() -> void;  ///< 空块删除

  auto SysyDelNoPreBLock() -> void;  ///< 删除unreachable块

  auto SysyAddReturn() -> void;  ///< 无后继的块添加return

  auto SysyOptimizateAll() -> void;  ///< 执行全部

  auto usedelete(Instruction *instr) -> void;  ///< 删除指令相关的use

  auto optimizationforop() -> void;  ///< optimization for cp

  auto SysyCondBr2BrPhi() -> void;  ///< condbr 1/0 to Br 在phi的条件下

  auto Sysy1phiDelete() -> void;  ///< phi指令只有一个的情况下删除

  auto SysyOpforCE() -> void;  ///< 在有phi的情况下进行CFG优化
};

}  // namespace sysy
