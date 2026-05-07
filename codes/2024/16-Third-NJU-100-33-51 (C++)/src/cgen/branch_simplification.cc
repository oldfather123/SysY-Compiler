#include "Machine.hh"
#include <unordered_set>
using std::unordered_set;

void thread_jmp(MFunction *func) {
  for (auto bb : func->getBasicBlocks()) {
    if (bb->getInstructions().size() == 0) { // blocks which can be ignored
      if (bb->getJmpNum() == 1) {
        auto jmp = bb->getJmps()[0];
        assert(jmp->getInsTag() == MInstruction::J);
        auto target = static_cast<MIj *>(jmp)->getTargetBB();
        // std::cout << " target is " << target->getName() << endl;

        vector<MBasicBlock*> incomings;
        //  std::cout << "    bb is " << bb->getName() << endl;
        for (auto prev : bb->getIncomings()) {
          // std::cout << "    prev addr is " << prev << endl;
          // std::cout << "    prev is " << prev->getName() << endl;
          incomings.push_back(prev);
        }
        for (auto prev : incomings) {
          target->removeIncoming(bb); // can be commented?
          prev->replaceOutgoing(bb, target);
        }
        // todo: remove dead bb
        // func->removeBasicBlock(bb);
      }
    }
  }
}

MInstruction *reverseConJmp(MInstruction *ins, MBasicBlock *target) {
  switch (ins->getInsTag()) {
  case MInstruction::BEQ: {
    auto beq = static_cast<MIbeq *>(ins);
    return new MIbne(beq->getReg(0), beq->getReg(1), target);
  }
  case MInstruction::BNE: {
    auto bne = static_cast<MIbne *>(ins);
    return new MIbeq(bne->getReg(0), bne->getReg(1), target);
  }
  case MInstruction::BGE: {
    auto bge = static_cast<MIbge *>(ins);
    return new MIblt(bge->getReg(0), bge->getReg(1), target);
  }
  case MInstruction::BLT: {
    auto blt = static_cast<MIblt *>(ins);
    return new MIbge(blt->getReg(0), blt->getReg(1), target);
  }
  default: {
    std::cout << ins->getInsTag() << endl;
    std::cout << *ins << endl;
    assert(0);
  }
  }
}

// hypothesis: prefer contiguous ; prefer loop
void orderBasicBlocksRecursive(MBasicBlock *bb,
                               vector<MBasicBlock *> &orderedBBs,
                               std::unordered_set<MBasicBlock *> &visited) {
  if (visited.count(bb) > 0) {
    return;
  }
  orderedBBs.push_back(bb);
  visited.insert(bb);
  auto outgoings = bb->getOutgoings();
  assert(outgoings.size() <= 2);
  if (outgoings.size() == 1) {
    orderBasicBlocksRecursive(outgoings[0], orderedBBs, visited);
  } else if (outgoings.size() == 2) {
    assert(bb->getJmpNum() == 2);
    auto indirect = getTargetOfJmp(bb->getJmp(0));
    auto direct = getTargetOfJmp(bb->getJmp(1));
    auto indirect_lp = bb->getFunction()->bbDepth->at(indirect);
    auto direct_lp = bb->getFunction()->bbDepth->at(direct);
    if (direct_lp >= indirect_lp) {
      orderBasicBlocksRecursive(direct, orderedBBs, visited);
      orderBasicBlocksRecursive(indirect, orderedBBs, visited);
    } else {
      auto conjmp = reverseConJmp(bb->getJmp(0), direct);
      auto jmp = new MIj(indirect);
      bb->clearJmps();
      bb->pushJmp(conjmp);
      bb->pushJmp(jmp);
      orderBasicBlocksRecursive(indirect, orderedBBs, visited);
      orderBasicBlocksRecursive(direct, orderedBBs, visited);
    }
  } else {
    // exit
  }
}

// todo: https://arxiv.org/pdf/1809.04676
void orderBasicBlocks(MFunction *func) {
  func->ordered_blocks = make_unique<vector<MBasicBlock *>>();
  unordered_set<MBasicBlock *> visited;
  orderBasicBlocksRecursive(func->getEntry(), *func->ordered_blocks, visited);

  for (int i = 0; i + 1 < func->ordered_blocks->size(); i++) {
    auto prev = func->ordered_blocks->at(i);
    auto next = func->ordered_blocks->at(i + 1);
    if (prev->getJmpNum() == 0)
      continue;
    auto lj = prev->getJmp(prev->getJmpNum() - 1);
    if (lj->getInsTag() == MInstruction::J && getTargetOfJmp(lj) == next) {
      lj->replaceWith({});
    }
  }

  func->getMod()->if_linearized = true;
}

void branch_simplify(MModule *mod) {
  for (auto func : mod->getFunctions()) {
    thread_jmp(func);
    orderBasicBlocks(func);
  }
}