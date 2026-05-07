/**
 * @file: SysyLoopspecificate.h
 * @brief  循环规范化
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */

#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"
namespace sysy {
/**
 * @brief  循环规范化
 */
class SysYLoopspecificator {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  /// @brief 构造函数
  SysYLoopspecificator(Module *pMoudle, IRBuilder *pBuilder) : pModule(pMoudle), pBuilder(pBuilder){};

  auto run() -> void;  ///< 执行接口

  auto Latch2One(Loop *toploop, Function *parentfunction) -> void;  ///< 将latch边归一

  auto exitBlockinsert(Loop *toploop, Function *parentfunction) -> void;  ///< exitblock变为只有一个前驱

  auto preHeaderOne(Loop *toploop, Function *parentfunction) -> void;  ///< preheader变为一个

  auto preIfBfloop(Loop *toploop, Function *parentfunction) -> void;  ///< 在loop前添加

  auto CpHeader(BasicBlock *header2cp, BasicBlock *cp2, int phiindex) -> void;
};
}  // namespace sysy