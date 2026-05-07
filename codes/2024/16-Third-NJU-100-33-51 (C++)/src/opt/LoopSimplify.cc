// Reference: https://llvm.org/docs/doxygen/LoopSimplify_8cpp_source.html

#include "LoopSimplify.hh"

bool LoopSimplify::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool LoopSimplify::runOnFunction(Function* func) {
  bool changed = false;
  LoopInfoBase* loopInfoBase = func->getLoopInfoBase();
  CFG* cfg = func->getCFG();
  if (!cfg) cfg = func->buildCFG();

  assert(loopInfoBase);
  for (LoopInfo* loopInfo : loopInfoBase->loopInfos) {
    if (needGuard) {
      changed |= simplyOneLoopWithGuard(loopInfo, cfg);
    } else {
      changed |= simplyOneLoop(loopInfo, cfg);
    }
  }

  if (changed) {
    func->resetDT();
  }
  return changed;
}

bool LoopSimplify::simplyOneLoopWithGuard(LoopInfo* loopInfo, CFG* cfg) {
  bool changed = false;

  // 1. remove dead block (Is it need?)

  // 2. build guard
  BasicBlock* header = loopInfo->header;
  vector<BasicBlock*> outsideBlocks;
  for (BasicBlock* pred : *cfg->getPredOf(header)) {
    if (!loopInfo->containBlock(pred)) {
      outsideBlocks.push_back(pred);
    }
  }
  if (!loopInfo->guard) {
    unordered_map<Value*, Value*> instrMap;
    BasicBlock* guard = header->splitBlockPredecessors(outsideBlocks, instrMap);
    loopInfo->guard = guard;
    changed = true;
    if (loopInfo->parentLoop) {
      loopInfo->parentLoop->addBlock(loopInfo->guard);
    }
    guard->getTailInstr()->eraseFromParent();
    BranchInst* headerBranch = (BranchInst*)loopInfo->header->getTailInstr();
    BasicBlock* inBlock = (BasicBlock*)headerBranch->getRValue(1);
    BasicBlock* exitBlock = (BasicBlock*)headerBranch->getRValue(2);
    instrMap.emplace(inBlock, loopInfo->header);
    if (loopInfo->containBlock(exitBlock)) {
      std::swap(inBlock, exitBlock);
    }
    for (Instruction* headerInstr : *header->getInstructions()) {
      if (headerInstr->isa(VT_PHI)) continue;
      Instruction* guardInstr = headerInstr->clone();
      guardInstr->cloneUseList(instrMap, headerInstr->getUseList());
      instrMap.emplace(headerInstr, guardInstr);
      guard->pushInstr(guardInstr);
    }

    cfg->addEdge(guard, exitBlock);

    vector<BasicBlock*> beforeExit;
    for (Instruction* instr : *exitBlock->getInstructions()) {
      if (PhiInst* phiInstr = dynamic_cast<PhiInst*>(instr)) {
        Value* fromHeader = 0;
        int icSize = phiInstr->getRValueSize() / 2;
        for (int i = 0; i < icSize; i++) {
          if ((BasicBlock*)phiInstr->getRValue(i * 2 + 1) == header) {
            fromHeader = phiInstr->getRValue(i * 2);
          }
        }
        assert(fromHeader);
        Value* fromGuard = fromHeader;
        auto it = instrMap.find(fromHeader);
        if (it != instrMap.end()) {
          fromGuard = it->second;
        }
        phiInstr->pushIncoming(fromGuard, guard);
      } else {
        break;
      }
    }
    for (Instruction* headerInstr : *header->getInstructions()) {
      vector<Use*> outsideUser;
      for (Use* use = headerInstr->getUseHead(); use; use = use->next) {
        Instruction* userInstr = use->instr;
        if (loopInfo->containBlockInChildren(userInstr->getParent())) continue;
        outsideUser.push_back(use);
      }
      if (outsideUser.empty()) continue;
      PhiInst* newPhi = new PhiInst(headerInstr->getName());
      for (BasicBlock* pred : *cfg->getPredOf(exitBlock)) {
        if (pred == guard) {
          newPhi->pushIncoming(instrMap[headerInstr], guard);
        } else {
          newPhi->pushIncoming(headerInstr, pred);
        }
      }

      exitBlock->pushInstrAtHead(newPhi);
      for (Use* user : outsideUser) {
        Instruction* userInstr = user->instr;
        if (!(userInstr->isa(VT_PHI) && userInstr->getParent() == exitBlock)) {
          user->replaceValue(newPhi);
        }
      }
    }
  }
  // 3. build preHeader
  vector<BasicBlock*> guardBlocks{loopInfo->guard};
  if (!loopInfo->preHeader) {
    unordered_map<Value*, Value*> nouse;
    loopInfo->preHeader = header->splitBlockPredecessors(guardBlocks, nouse);
    changed = true;
    if (loopInfo->parentLoop) {
      loopInfo->parentLoop->addBlock(loopInfo->preHeader);
    }
  }

  return changed;
}

bool LoopSimplify::simplyOneLoop(LoopInfo* loopInfo, CFG* cfg) {
  bool changed = false;

  // 1. remove dead block (Is it need?)

  // 2. build preHeader
  BasicBlock* header = loopInfo->header;
  vector<BasicBlock*> outsideBlocks;
  for (BasicBlock* pred : *cfg->getPredOf(header)) {
    if (!loopInfo->containBlock(pred)) {
      outsideBlocks.push_back(pred);
    }
  }
  if (!loopInfo->preHeader) {
    unordered_map<Value*, Value*> nouse;
    loopInfo->preHeader = header->splitBlockPredecessors(outsideBlocks, nouse);
    changed = true;
    if (loopInfo->parentLoop) {
      loopInfo->parentLoop->addBlock(loopInfo->preHeader);
    }
  }

  return changed;
}
