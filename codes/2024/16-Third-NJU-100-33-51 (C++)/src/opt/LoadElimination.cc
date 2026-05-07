#include "LoadElimination.hh"

bool LoadElimination::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool LoadElimination::runOnFunction(Function* func) {
  bool changed = false;
  AliasAnalysisResult* aaResult = func->getAliasAnalysisResult();
  assert(aaResult);
  for (BasicBlock* block : *func->getBasicBlocks()) {
    changed |= runOnBasicBlock(block, aaResult);
  }
  return changed;
}

bool LoadElimination::runOnBasicBlock(BasicBlock* bb,
                                      AliasAnalysisResult* aaResult) {
  unordered_map<Value*, Value*> memToValue;
  unordered_set<Instruction*> trashLoads;
  for (Instruction* instr : *bb->getInstructions()) {
    switch (instr->getValueTag()) {
      case VT_STORE: {
        Value* mem = instr->getRValue(1);
        unordered_set<Value*> eraseSet;
        for (auto& [preMen, preValue] : memToValue) {
          if (!aaResult->isDistinct(preMen, mem)) {
            eraseSet.insert(preMen);
          }
        }
        for (auto& eraseMem : eraseSet) {
          memToValue.erase(eraseMem);
        }
        memToValue[mem] = instr->getRValue(0);
        break;
      }
      case VT_LOAD: {
        Value* mem = instr->getRValue(0);
        auto it = memToValue.find(mem);
        if (it == memToValue.end()) {
          memToValue[mem] = instr;
          continue;
        }
        instr->replaceAllUsesWith(it->second);
        trashLoads.insert(instr);
        break;
      }
      case VT_CALL: {
        CallInst* callInst = (CallInst*)instr;
        Function* callee = callInst->getFunction();
        if (callee->hasMemWrite()) {
          memToValue.clear();
        }
        break;
      }
    }
  }

  for (Instruction* trash : trashLoads) {
    trash->eraseFromParent();
    trash->deleteUseList();
  }
  return !trashLoads.empty();
}