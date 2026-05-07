#include "GEPSimplify.hh"

bool GEPSimplify::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool GEPSimplify::runOnFunction(Function* func) {
  bool changed = false;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    changed |= runOnBasicBlock(block);
  }
  return changed;
}

bool GEPSimplify::runOnBasicBlock(BasicBlock* block) {
  unordered_map<Value*, std::pair<GetElemPtrInst*, GetElemPtrInst*>>
      preInstrMap;
  vector<std::pair<GetElemPtrInst*, GetElemPtrInst*>> gepPairs;

  // gep addr, 0, idx
  for (Instruction* instr : *block->getInstructions()) {
    GetElemPtrInst* oldGep = dynamic_cast<GetElemPtrInst*>(instr);
    if (!oldGep || oldGep->getRValueSize() != 3) continue;
    Value* loc = oldGep->getRValue(0);
    auto it = preInstrMap.find(loc);
    if (it == preInstrMap.end()) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    auto preOldGep = it->second.first;
    auto preNewGep = it->second.second;

    if (preOldGep->getRValue(0) != oldGep->getRValue(0) ||
        preOldGep->getRValue(1) != oldGep->getRValue(1)) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    BinaryOpInst* strideInstr =
        dynamic_cast<BinaryOpInst*>(oldGep->getRValue(2));
    if (!strideInstr || strideInstr->getOpTag() != ADD ||
        strideInstr->getRValue(0) != preOldGep->getRValue(2)) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    GetElemPtrInst* newGep =
        new GetElemPtrInst(preNewGep, strideInstr->getRValue(1), "new.gep");
    gepPairs.emplace_back(oldGep, newGep);
    preInstrMap[loc] = {oldGep, newGep};
  }


  // gep addr, idx
  preInstrMap.clear();
  for (Instruction* instr : *block->getInstructions()) {
    GetElemPtrInst* oldGep = dynamic_cast<GetElemPtrInst*>(instr);
    if (!oldGep || oldGep->getRValueSize() != 2) continue;
    Value* loc = oldGep->getRValue(0);
    auto it = preInstrMap.find(loc);
    if (it == preInstrMap.end()) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    auto preOldGep = it->second.first;
    auto preNewGep = it->second.second;

    if (preOldGep->getRValue(0) != oldGep->getRValue(0)) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    BinaryOpInst* strideInstr =
        dynamic_cast<BinaryOpInst*>(oldGep->getRValue(1));
    if (!strideInstr || strideInstr->getOpTag() != ADD ||
        strideInstr->getRValue(0) != preOldGep->getRValue(1)) {
      preInstrMap[loc] = {oldGep, oldGep};
      continue;
    }
    GetElemPtrInst* newGep =
        new GetElemPtrInst(preNewGep, strideInstr->getRValue(1), "new.gep");
    gepPairs.emplace_back(oldGep, newGep);
    preInstrMap[loc] = {oldGep, newGep};
  }

  for (auto [oldGep, newGep] : gepPairs) {
    newGep->moveBefore(oldGep);
    oldGep->replaceAllUsesWith(newGep);
    oldGep->eraseFromParent();
    oldGep->deleteUseList();
    delete oldGep;
  }
  return !gepPairs.empty();
}