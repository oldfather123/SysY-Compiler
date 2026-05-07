#include "passes.h"

namespace pass {

bool BranchOpti::onFunction(ir::Function *function) {
  bool changed = false;
  for (int i = 1; i < function->size(); i++) {
    const auto block = function->getBlock(i);
    const auto &uses = block->getUses();
    if (uses.empty()) {
      // Remove the block if it has no uses
      if (const auto branchInst =
              dynamic_cast<ir::BranchInst *>(block->getLastInstruction())) {
        std::unordered_set<ir::BasicBlock *> nextBlocks;
        if (branchInst->isConditional()) {
          nextBlocks.insert(
              static_cast<ir::BasicBlock *>(branchInst->getOperand(1)));
          nextBlocks.insert(
              static_cast<ir::BasicBlock *>(branchInst->getOperand(2)));
        } else {
          nextBlocks.insert(
              static_cast<ir::BasicBlock *>(branchInst->getOperand(0)));
        }
        for (const auto nextBlock : nextBlocks) {
          for (const auto &instPtr : *nextBlock) {
            auto inst = static_cast<ir::Instruction *>(instPtr.get());
            if (const auto phiNode = dynamic_cast<ir::PhiInst *>(inst)) {
              for (int j = 0; j < phiNode->getNumOperands(); j++) {
                if (phiNode->getIncomingBlock(j) == block) {
                  phiNode->removeIncoming(j);
                  j--;
                }
              }
            }
          }
        }
      }
      function->eraseBlock(i);
      i--;
      changed = true;
    } else if (uses.size() == 1) {
      auto it = uses.begin();
      const auto branchInst = dynamic_cast<ir::BranchInst *>((*it)->getUser());
      if (!branchInst->isConditional()) {
        auto prevBlock = static_cast<ir::BasicBlock *>(branchInst->getBlock());
        if (const auto nextBranchInst =
                dynamic_cast<ir::BranchInst *>(block->getLastInstruction())) {
          std::unordered_set<ir::BasicBlock *> nextBlocks;
          if (nextBranchInst->isConditional()) {
            nextBlocks.insert(
                static_cast<ir::BasicBlock *>(nextBranchInst->getOperand(1)));
            nextBlocks.insert(
                static_cast<ir::BasicBlock *>(nextBranchInst->getOperand(2)));
          } else {
            nextBlocks.insert(
                static_cast<ir::BasicBlock *>(nextBranchInst->getOperand(0)));
          }
          for (const auto nextBlock : nextBlocks) {
            for (const auto &instPtr : *nextBlock) {
              auto inst = static_cast<ir::Instruction *>(instPtr.get());
              if (const auto phiNode = dynamic_cast<ir::PhiInst *>(inst)) {
                for (int j = 0; j < phiNode->getNumOperands(); j++) {
                  if (phiNode->getIncomingBlock(j) == block) {
                    phiNode->setIncomingBlock(j, prevBlock);
                  }
                }
              }
            }
          }
        }
        prevBlock->eraseInstruction(static_cast<int>(prevBlock->size()) - 1);
        for (size_t i = 0; i < block->size(); i++) {
          auto erased = block->getOwnership(i);
          prevBlock->pushInstruction(std::move(erased));
        }
        function->eraseBlock(i);
        i--;
        changed = true;
      }
    }
  }
  for (int i = 0; i < function->size(); i++) {
    auto *block = static_cast<ir::BasicBlock *>(function->getBlock(i));
    const auto branchInst =
        dynamic_cast<ir::BranchInst *>(block->getLastInstruction());
    if (branchInst && branchInst->isConditional()) {
      if (const auto condValue =
              dynamic_cast<ir::ConstantNumber *>(branchInst->getOperand(0))) {
        auto cond = condValue->intValue();
        assert(cond == 0 || cond == 1);
        branchInst->eraseOperand(0);
        auto use = branchInst->eraseOperand(cond);
        const auto nextBlock = dynamic_cast<ir::BasicBlock *>(use->getValue());
        for (const auto &instPtr : *nextBlock) {
          auto inst = static_cast<ir::Instruction *>(instPtr.get());
          if (const auto phiNode = dynamic_cast<ir::PhiInst *>(inst)) {
            for (int j = 0; j < phiNode->getNumOperands(); j++) {
              if (phiNode->getIncomingBlock(j) == block) {
                phiNode->removeIncoming(j);
                j--;
              }
            }
          }
        }
        changed = true;
      }
    }
  }
  return changed;
}

} // namespace pass