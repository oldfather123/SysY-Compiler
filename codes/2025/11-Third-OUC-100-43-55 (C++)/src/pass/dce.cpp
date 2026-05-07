#include "passes.h"

namespace pass {

bool DeadCodeElim::onFunction(ir::Function *function) {
  bool changed = false;
  for (auto &blockPtr : *function) {
    auto block = static_cast<ir::BlockBase *>(blockPtr.get());
    for (size_t i = 0; i + 1 < block->size(); i++) {
      auto inst = static_cast<ir::Instruction *>(block->getInstruction(i));
      auto instKind = inst->getInstKind();
      if ((instKind != ir::InstKind::Call) &&
          (instKind != ir::InstKind::Store) &&
          (instKind != ir::InstKind::Ret) && inst->getUses().empty()) {
        block->eraseInstruction(i);
        i--;
        changed = true;
      }
    }
  }
  return changed;
}

} // namespace pass
