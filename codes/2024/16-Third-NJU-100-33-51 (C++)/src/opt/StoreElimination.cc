#include "StoreElimination.hh"

bool StoreElimination::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }
  return changed;
}

bool StoreElimination::runOnFunction(Function* func) {
  bool changed = false;
  AliasAnalysisResult* aaResult = func->getAliasAnalysisResult();
  assert(aaResult);
  for (BasicBlock* block : *func->getBasicBlocks()) {
    changed |= runOnBasicBlock(block, aaResult);
  }
  changed |= eliminationLocalArray(func);
  return changed;
}

bool StoreElimination::runOnBasicBlock(BasicBlock* block,
                                       AliasAnalysisResult* aaResult) {
  unordered_map<Value*, Instruction*> memToStore;
  unordered_set<Instruction*> trashStores;
  for (Instruction* instr : *block->getInstructions()) {
    switch (instr->getValueTag()) {
      case VT_STORE: {
        Value* mem = instr->getRValue(1);
        auto it = memToStore.find(mem);
        if (it != memToStore.end()) {
          trashStores.insert(it->second);
          memToStore[mem] = instr;
        } else {
          memToStore[mem] = instr;
        }
        break;
      }
      case VT_LOAD: {
        Value* mem = instr->getRValue(0);
        unordered_set<Value*> eraseSet;
        for (auto& [preMem, preStore] : memToStore) {
          if (!aaResult->isDistinct(preMem, mem)) {
            eraseSet.insert(preMem);
          }
        }
        for (auto& eraseMem : eraseSet) {
          memToStore.erase(eraseMem);
        }
        break;
      }
      case VT_CALL: {
        memToStore.clear();
        break;
      }
    }
  }

  for (Instruction* trash : trashStores) {
    trash->eraseFromParent();
    trash->deleteUseList();
  }
  return !trashStores.empty();
}

// If there aren't any load from a local array, and it's not a argument of
// any call then all store to it can be eliminate
bool StoreElimination::eliminationLocalArray(Function* func) {
  bool changed = false;
  BasicBlock* entry = func->getEntry();
  for (Instruction* instr : *entry->getInstructions()) {
    if (!instr->isa(VT_ALLOCA)) continue;
    AllocaInst* alloca = (AllocaInst*)instr;
    if (alloca->getElemType()->getTypeTag() != TT_ARRAY) continue;
    bool flag = true;
    LinkedList<Instruction*> worklist;
    vector<StoreInst*> stores;
    worklist.pushBack(alloca);
    while (!worklist.isEmpty()) {
      Instruction* curInstr = worklist.popFront();
      switch (curInstr->getValueTag()) {
        case VT_ALLOCA:
        case VT_GEP: {
          for (Use* use = curInstr->getUseHead(); use; use = use->next) {
            Instruction* userInstr = use->instr;
            worklist.pushBack(userInstr);
          }
          break;
        }
        case VT_CALL:
        case VT_LOAD: {
          flag = false;
          break;
        }
        case VT_STORE: {
          stores.push_back((StoreInst*)curInstr);
          break;
        }
        default:
          assert(0);
      }
      if (!flag) break;
    }
    if (flag) {
      changed = true;
      for (auto store: stores) {
        store->eraseFromParent();
        store->deleteUseList();
        delete store;
      }
    }
  }
  return changed;
}
