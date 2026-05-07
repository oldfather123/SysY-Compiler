#include "DeadArgumentElimination.hh"

bool DeadArgumentElimination::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }
  return changed;
}

bool DeadArgumentElimination::runOnFunction(Function* func) {
  FuncType* funcType = (FuncType*)func->getType();
  int argSize = funcType->getArgSize();
  vector<int> deadArgIndex;
  for (int i = 0; i < argSize; i++) {
    Argument* arg = funcType->getArgument(i);
    if (arg->getUserSize() == 0) {
      deadArgIndex.push_back(i);
    }
  }

  if (deadArgIndex.empty()) return false;

  std::reverse(deadArgIndex.begin(), deadArgIndex.end());
  for (int loc : deadArgIndex) {
    funcType->deleteArgumentAt(loc);
  }

  for (CallInst* callSite : *func->getCallSites()) {
    for (int loc : deadArgIndex) {
      callSite->deleteRValueAt(loc);
    }
  }

  return true;
}
