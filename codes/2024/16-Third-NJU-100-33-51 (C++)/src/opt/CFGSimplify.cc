#include "CFGSimplify.hh"

bool CFGSimplify::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }
  return changed;
}
bool CFGSimplify::runOnFunction(Function* func) {
  bool changed = false;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    // IF
    // A -> B -> C
    // B is empty
    // No PHI in C
    // =>
    // A -> C
    if (block->getInstrSize() == 1 && block->getTailInstr()->isa(VT_JUMP)) {
      JumpInst* jump = (JumpInst*)block->getTailInstr();
      BasicBlock* nextBlock = (BasicBlock*)jump->getRValue(0);
      if (!nextBlock->getInstructionAt(0)->isa(VT_PHI)) {
        block->replaceAllUsesWith(nextBlock);
        changed = true;
        continue;
      }
    }
    // br cond, L1, L1
    // =>
    // jump L1
    // (no use)
    if (block->getTailInstr() && block->getTailInstr()->isa(VT_BR)) {
      BranchInst* branch = (BranchInst*)block->getTailInstr();
      if (branch->getRValue(1) == branch->getRValue(2)) {
        JumpInst* jump = new JumpInst((BasicBlock*)branch->getRValue(1));
        branch->eraseFromParent();
        branch->deleteUseList();
        block->pushInstr(jump);
        changed = true;
        continue;
      }
    }
  }
  if (changed) {
    func->resetCFG();
    func->resetDT();
  }
  return false;
}