#include "MergeBlock.hh"
bool MergeBlock::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

/**
 * 1. block and succBlock is one to one
 * 2. block != succblock
 * 3. (Maybe?) PHI loop, i.e., x1 = phi(x1, x2), is it possible?
 * Attention: CFG, DT, DF will be changed in this pass
 */
bool MergeBlock::runOnFunction(Function* func) {
  bool changed = false;
  CFG* cfg = func->getCFG();
  if (!cfg) {
    cfg = func->buildCFG();
  }
  LinkedList<BasicBlock*> trashList;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    // this block has been merge
    if (!block->getInstructions()->getSize()) {
      continue;
    }
    while (cfg->getSuccOf(block)->getSize() == 1) {
      BasicBlock* succBlock = cfg->getSuccOf(block)->front();
      // have multiple pred block or self loop
      if (cfg->getPredOf(succBlock)->getSize() != 1 || succBlock == block ||
          succBlock == func->getExit()) {
        break;
      }

      // Now block and succBlock can be merged
      changed = true;
      // remove tail instruction
      Instruction* tail = block->getTailInstr();
      if (tail->isa(VT_RET)) break;
      tail->eraseFromParent();
      tail->deleteUseList();
      delete tail;
      // move instruction from succBlock to block
      for (Instruction* instr : *succBlock->getInstructions()) {
        if (instr->isa(VT_PHI)) {
          // it must have only one incoming value from block
          assert(instr->getRValueSize() == 2);
          assert((BasicBlock*)instr->getRValue(1) == block);
          PhiInst* phi = dynamic_cast<PhiInst*>(instr);
          phi->replaceAllUsesWith(instr->getRValue(0));
          phi->deleteUseList();
          delete phi;
        } else {
          block->pushInstr(instr);
        }
      }
      succBlock->getInstructions()->clear();
      succBlock->replaceAllUsesWith(block);
      trashList.pushBack(succBlock);
      // Modify CFG
      for (BasicBlock* ssuccBlock : *cfg->getSuccOf(succBlock)) {
        cfg->addEdge(block, ssuccBlock);
      }
      cfg->eraseNode(succBlock);

      // Modify DT
      DomTree* dt = func->getDT();
      if (!dt) continue;
      assert(dt->getDominator(succBlock) == block);
      dt->deleteChildren(block);
      dt->deleteParent(succBlock);
      dt->mergeChildrenTo(succBlock, block);
      dt->eraseNode(succBlock);
    }
  }
  for (BasicBlock* block : trashList) {
    block->eraseFromParent();
  }
  return changed;
}
