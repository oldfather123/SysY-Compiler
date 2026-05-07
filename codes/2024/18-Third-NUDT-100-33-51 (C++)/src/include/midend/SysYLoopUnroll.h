/**
 * @file: SysYLoopUnroll.h
 * @brief  循环展开
 * @Author : Ixeux email:you@domain.com
 * @Version : 1.0
 * @Creat Date : 2024-08-10
 *
 */
#include <list>
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"
namespace sysy {
/**
 * @brief  循环展开类
 */
class SysYLoopUnroll {
 private:
  Module *pModule;
  IRBuilder *pBuilder;

 public:
  SysYLoopUnroll(Module *pModule, IRBuilder *pBuilder) : pModule(pModule), pBuilder(pBuilder) {}

  auto UnRollforLoop(Loop *toploop) -> void;  ///< 将循环进行展开

  auto CpBlocks(std::list<BasicBlock *> &Blocks2Cp, Loop *toploop, std::string time, BasicBlock *oldlatch,
                int headerBlockMode) -> std::pair<BasicBlock *, BasicBlock *>;  ///< 复制块

  auto usedelete(Instruction *instr) -> void;  ///< 删除该指令的UD关系

  auto run() -> void;  ///< 执行接口
};
}  // namespace sysy
