#include "DeadCodeElimination.hh"

bool DeadCodeElimination::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }

  // remove unuse global variable
  auto gvList = module->getGlobalVariables();
  vector<GlobalVariable*> trashList;
  for (GlobalVariable* gv : *gvList) {
    if (gv->getUseHead() == nullptr) {
      trashList.push_back(gv);
    }
  }
  for (GlobalVariable* gv : trashList) {
    gvList->remove(gv);
  }
  return changed || !gvList->isEmpty();
}

bool DeadCodeElimination::runOnFunction(Function* func) {
  bool changed = false;
  changed |= simplifyInstruction(func);
  changed |= eliminateDeadBlocks(func);
  changed |= eliminateDeadInstructions(func);
  changed |= simplifyInstruction(func);

  return changed;
}

bool DeadCodeElimination::eliminateDeadBlocks(Function* func) {
  CFG* cfg = func->getCFG();
  if (!cfg) cfg = func->buildCFG();
  unordered_set<BasicBlock*> deadBlocks;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    deadBlocks.insert(block);
  }

  // bfs on cfg
  LinkedList<BasicBlock*> worklist;
  worklist.pushBack(cfg->getEntry());
  while (!worklist.isEmpty()) {
    BasicBlock* block = worklist.popFront();
    if (deadBlocks.count(block) == 0) continue;
    for (BasicBlock* succ : *cfg->getSuccOf(block)) {
      worklist.pushBack(succ);
    }
    deadBlocks.erase(block);
  }

  for (BasicBlock* deadBlock : deadBlocks) {
    if (deadBlock == func->getExit()) continue;

    for (Instruction* instr : *deadBlock->getInstructions()) {
      instr->deleteUseList();
    }

    // delete phi incoming from deadBlock
    for (Use* use = deadBlock->getUseHead(); use;) {
      Instruction* userInstr = use->instr;
      use = use->next;
      if (PhiInst* phi = dynamic_cast<PhiInst*>(userInstr)) {
        phi->deleteIncomingFrom(deadBlock);
      }
    }
    cfg->eraseNode(deadBlock);
    deadBlock->eraseFromParent();
    delete deadBlock;
  }
  bool changed = !deadBlocks.empty();
  if (changed) {
    func->resetCFG();
    func->resetDT();
  }
  return changed;
}
Value* tmp = 0;
bool DeadCodeElimination::eliminateDeadInstructions(Function* func) {
  LinkedList<Instruction*> worklist;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    for (Instruction* instr : *block->getInstructions()) {
      if (!instr->getUseHead() && isComputeInstruction(instr)) {
        // No user
        worklist.pushBack(instr);
      }
      if (instr->getUserSize() == 1 &&
          instr->getUseHead()->instr->isa(VT_PHI)) {
        Instruction* userPtr = instr->getUseHead()->instr;
        bool flag = false;
        unordered_set<Instruction*> visited;
        visited.insert(instr);
        while (isComputeInstruction(userPtr)) {
          if (visited.count(userPtr)) {
            // Find a ring
            int opSize = userPtr->getRValueSize();
            for (int i = 0; i < opSize; i++) {
              Instruction* op =
                  dynamic_cast<Instruction*>(userPtr->getRValue(i));
              if (op && isComputeInstruction(op)) {
                worklist.pushBack(op);
              }
            }
            userPtr->deleteUseList();
            break;
          }
          visited.insert(userPtr);
          if (userPtr->getUserSize() != 1) {
            flag = true;
            break;
          }
          userPtr = userPtr->getUseHead()->instr;
        }
        if (!flag) {
          worklist.pushBack(instr);
        }
      }
    }
  }
  bool changed = !worklist.isEmpty();

  LinkedList<Instruction*> trashList;

  while (!worklist.isEmpty()) {
    Instruction* deadInst = worklist.popFront();
    if (deadInst->getUseHead() || !deadInst->getParent()) continue;
    int argSize = deadInst->getRValueSize();
    for (int i = 0; i < argSize; i++) {
      Value* rValue = deadInst->getRValue(i);
      tmp = rValue;
      if (Instruction* effectInst = dynamic_cast<Instruction*>(rValue)) {
        if (isComputeInstruction(effectInst)) {
          worklist.pushBack(effectInst);
        }
      }
    }
    deadInst->eraseFromParent();
    deadInst->deleteUseList();
    trashList.pushBack(deadInst);
  }
  for (Instruction* trash : trashList) {
    delete trash;
  }

  return changed;
}

bool DeadCodeElimination::simplifyInstruction(Function* func) {
  bool changed = false;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    // Cond branch to uncond branch
    if (BranchInst* branch = dynamic_cast<BranchInst*>(block->getTailInstr())) {
      Value* cond = branch->getRValue(0);
      if (BoolConstant* boolConstant = dynamic_cast<BoolConstant*>(cond)) {
        changed = true;
        BasicBlock* succBlock = nullptr;
        BasicBlock* unreachBLock = nullptr;
        if (boolConstant->getValue()) {
          succBlock = (BasicBlock*)branch->getRValue(1);
          unreachBLock = (BasicBlock*)branch->getRValue(2);
        } else {
          succBlock = (BasicBlock*)branch->getRValue(2);
          unreachBLock = (BasicBlock*)branch->getRValue(1);
        }
        if (func->getCFG() && unreachBLock != succBlock) {
          func->getCFG()->eraseEdge(block, unreachBLock);
        }
        branch->eraseFromParent();
        branch->deleteUseList();
        block->pushInstr(new JumpInst(succBlock));
        delete branch;

        // delete phi incoming from block to unreachBlock
        for (Instruction* instr : *unreachBLock->getInstructions()) {
          if (PhiInst* phiInstr = dynamic_cast<PhiInst*>(instr)) {
            phiInstr->deleteIncomingFrom(block);
          }
        }
      }
    }

    if (changed) {
      func->resetDT();
    }

    // delete single entry phi
    PhiInst* phiInst = 0;
    for (auto it = block->getInstructions()->begin();
         it != block->getInstructions()->end();) {
      Instruction* instr = *it;
      ++it;
      if (phiInst = dynamic_cast<PhiInst*>(instr)) {
        int icSize = instr->getRValueSize() / 2;
        unordered_set<Value*> vals;
        for (int i = 0; i < icSize; i++) {
          vals.insert(instr->getRValue(i * 2));
        }
        if (vals.size() == 1) {
          changed = true;
          phiInst->replaceAllUsesWith(phiInst->getRValue(0));
          phiInst->eraseFromParent();
          phiInst->deleteUseList();
        }
      } else {
        break;
      }
    }
  }
  return changed;
}

bool DeadCodeElimination::isComputeInstruction(Instruction* instr) {
  if (instr->isa(VT_JUMP) || instr->isa(VT_BR) || instr->isa(VT_RET) ||
      instr->isa(VT_STORE) || instr->isa(VT_CALL)) {
    return false;
  }
  return true;
}
