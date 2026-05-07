/**
 * @File Name: SysyPhiDelete.h
 * @brief  phi删除 reg2mem
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#pragma once

#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"

namespace sysy {
/**
 * @brief  reg2mem
 */
class SysyPhiDelete {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  // 初始化函数
  SysyPhiDelete(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder) {}

  auto PhiDelete() -> void;                    ///< 删除phi
  auto usedelete(Instruction *instr) -> void;  ///< 删除UD关系
};

}  // namespace sysy
