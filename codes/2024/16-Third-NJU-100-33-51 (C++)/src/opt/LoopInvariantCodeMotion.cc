#include "LoopInvariantCodeMotion.hh"

bool LoopInvariantCodeMotion::isInvariant(
    Value* value, LoopInfo* loopInfo, unordered_set<Instruction*>& invarients) {
  Instruction* instr = nullptr;
  if (!(instr = dynamic_cast<Instruction*>(value))) return true;

  if (invarients.count(instr)) return true;

  BasicBlock* block = instr->getParent();
  if (loopInfo->containBlockInChildren(block)) return false;

  return true;
}

bool LoopInvariantCodeMotion::isMovable(Instruction* instr, LoopInfo* loopInfo,
                                        unordered_set<Instruction*>& invarients,
                                        AliasAnalysisResult* aaResult) {
  auto mayContainStoreTo = [&](Value* addr) {
    LinkedList<LoopInfo*> workList;
    workList.pushBack(loopInfo);
    while (!workList.isEmpty()) {
      LoopInfo* curLoop = workList.popFront();
      for (BasicBlock* subBlock : curLoop->blocks) {
        for (Instruction* subInstr : *subBlock->getInstructions()) {
          if (subInstr->isa(VT_STORE) &&
              !aaResult->isDistinct(addr, subInstr->getRValue(1)))
            return true;

          if (subInstr->isa(VT_CALL)) {
            CallInst* callInstr = dynamic_cast<CallInst*>(subInstr);
            Function* callee =
                dynamic_cast<Function*>(callInstr->getFunction());
            if (callee->hasMemWrite()) return true;
          }
        }
      }

      for (LoopInfo* subLoop : curLoop->subLoops) {
        workList.pushBack(subLoop);
      }
    }
    return false;
  };

  auto mayContainLoadFrom = [&](Value* addr) {
    LinkedList<LoopInfo*> workList;
    workList.pushBack(loopInfo);
    while (!workList.isEmpty()) {
      LoopInfo* curLoop = workList.popFront();
      for (BasicBlock* subBlock : curLoop->blocks) {
        for (Instruction* subInstr : *subBlock->getInstructions()) {
          if (subInstr->isa(VT_LOAD) &&
              !aaResult->isDistinct(addr, subInstr->getRValue(0)))
            return true;

          if (subInstr->isa(VT_CALL)) {
            CallInst* callInstr = dynamic_cast<CallInst*>(subInstr);
            Function* callee =
                dynamic_cast<Function*>(callInstr->getFunction());
            if (callee->hasMemRead()) return true;
          }
        }
      }

      for (LoopInfo* subLoop : curLoop->subLoops) {
        workList.pushBack(subLoop);
      }
    }
    return false;
  };

  Function* func = instr->getParent()->getParent();
  DomTree* dt = func->getDT();
  if (!dt) dt = func->buildDT();

  switch (instr->getValueTag()) {
    case VT_ALLOCA:
    case VT_BR:
    case VT_JUMP:
    case VT_PHI:
    case VT_RET:
      return false;

    case VT_LOAD: {
      if (mayContainStoreTo(instr->getRValue(0))) return false;
      break;
    }
    case VT_STORE: {
      if (mayContainLoadFrom(instr->getRValue(1))) return false;
      if (!dt->dominates(instr->getParent(), *(loopInfo->latches.begin())))
        return false;
      break;
    }

    case VT_CALL: {
      CallInst* callInstr = dynamic_cast<CallInst*>(instr);
      Function* callee = dynamic_cast<Function*>(callInstr->getFunction());
      if (!callee->isPureFunction()) return false;
    }
    default:
      break;
  }

  int rvSize = instr->getRValueSize();
  for (int i = 0; i < rvSize; i++) {
    if (!isInvariant(instr->getRValue(i), loopInfo, invarients)) return false;
  }
  return true;
}

bool LoopInvariantCodeMotion::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }
  return changed;
}

bool LoopInvariantCodeMotion::runOnFunction(Function* func) {
  bool changed = false;
  AliasAnalysisResult* aaResult = func->getAliasAnalysisResult();
  assert(aaResult);
  LoopInfoBase* liBase = func->getLoopInfoBase();
  for (LoopInfo* loopInfo : liBase->loopInfos) {
    // Deal loop from top to down
    if (loopInfo->parentLoop == nullptr) {
      changed |= runOnOneLoop(loopInfo, aaResult);
    }
  }
  return changed;
}

bool LoopInvariantCodeMotion::runOnOneLoop(LoopInfo* loopInfo,
                                           AliasAnalysisResult* aaResult) {
  bool changed = false;
  // Post order
  for (LoopInfo* subloop : loopInfo->subLoops) {
    changed |= runOnOneLoop(subloop, aaResult);
  }

  vector<Instruction*> invariant;
  unordered_set<Instruction*> invariantSet;

  LinkedList<Instruction*> worklist;
  for (BasicBlock* block : loopInfo->blocks) {
    for (Instruction* instr : *block->getInstructions()) {
      worklist.pushBack(instr);
    }
  }

  while (!worklist.isEmpty()) {
    Instruction* curInstr = worklist.popFront();
    if (isMovable(curInstr, loopInfo, invariantSet, aaResult)) {
      invariant.push_back(curInstr);
      invariantSet.insert(curInstr);
      for (Use* use = curInstr->getUseHead(); use; use = use->next) {
        Instruction* effectInstr = use->instr;
        if (loopInfo->containBlock(effectInstr->getParent())) {
          worklist.pushBack(effectInstr);
        }
      }
    }
  }

  // Move instruction to preHeader
  if (!invariant.empty()) {
    BasicBlock* preHeader = loopInfo->preHeader;
    assert(preHeader);
    Instruction* tailInstr = preHeader->getTailInstr();
    tailInstr->eraseFromParent();
    for (Instruction* moveInstr : invariant) {
      moveInstr->eraseFromParent();
      preHeader->pushInstr(moveInstr);
    }
    preHeader->pushInstr(tailInstr);
    return true;
  }
  return false;
}
