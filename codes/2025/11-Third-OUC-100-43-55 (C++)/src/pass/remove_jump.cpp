#include "passes.h"

namespace pass {

bool RemoveJump::onFunction(riscv::MachineFunc *function) {
  bool changed = false;
  for (size_t i = 0; i < function->size() - 1; ++i) {
    auto block = static_cast<riscv::MachineBlock *>(function->getBlock(i));
    auto nextBlock =
        static_cast<riscv::MachineBlock *>(function->getBlock(i + 1));
    if (block->hasUncondJump()) {
      auto jump = block->getUncondJump();
      auto target = jump->getTargetBlock();
      if (target == nextBlock) {
        block->eraseInstruction(block->size() -
                                1); // Remove the jump instruction
        changed = true;
      }
    }
  }
  return changed;
}

} // namespace pass
