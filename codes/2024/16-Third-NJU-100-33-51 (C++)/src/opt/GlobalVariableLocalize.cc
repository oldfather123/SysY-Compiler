#include "GlobalVariableLocalize.hh"
#include "MemToReg.hh"
bool GlobalVariableLocalize::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  changedFunctions.clear();
  for (GlobalVariable* gv : *module->getGlobalVariables()) {
    if (gv->getInitValue()->getType()->getTypeTag() != TT_ARRAY) {
      changed |= localize(gv);
    }
  }
  MemToReg* memToReg = new MemToReg();
  for (Function* func: changedFunctions) {
    memToReg->runOnFunction(func);
  }
  return changed;
}

bool GlobalVariableLocalize::runOnFunction(Function* func) { return false; }

// Only try to localize integer and float global variable
bool GlobalVariableLocalize::localize(GlobalVariable* gv) {
  unordered_map<Function*, unordered_set<Instruction*>> funcToInstrs;
  bool hasStore = false;
  for (Use* use = gv->getUseHead(); use; use = use->next) {
    Instruction* userInstr = use->instr;
    if (userInstr->isa(VT_STORE)) {
      hasStore = true;
    }
    Function* func = userInstr->getParent()->getParent();
    funcToInstrs[func].insert(userInstr);
  }

  if (funcToInstrs.empty()) return false;

  Value* initValue = gv->getInitValue();

  // no store to gloabl variable, replace it with constant
  if (!hasStore) {
    for (auto& [func, instrs] : funcToInstrs) {
      for (auto instr : instrs) {
        assert(instr->isa(VT_LOAD));
        LoadInst* loadInstr = static_cast<LoadInst*>(instr);
        loadInstr->replaceAllUsesWith(initValue);
        loadInstr->eraseFromParent();
        loadInstr->deleteUseList();
      }
    }
    return true;
  }

  // If global variable is only used in the main function,
  // convert it to a local variable
  if (funcToInstrs.size() == 1 &&
      funcToInstrs.begin()->first->getName() == "main") {
    Function* mainFunc = funcToInstrs.begin()->first;
    AllocaInst* alloca = new AllocaInst(initValue->getType(), gv->getName());
    StoreInst* store = new StoreInst(initValue, alloca);
    BasicBlock* entry = mainFunc->getEntry();
    entry->pushInstrAtHead(store);
    entry->pushInstrAtHead(alloca);
    gv->replaceAllUsesWith(alloca);
    changedFunctions.insert(mainFunc);
    return true;
  }

  bool changed = false;
  unordered_set<Function*> userFunctions;
  LinkedList<Function*> worklist;
  for (auto& [func, instrs] : funcToInstrs) {
    worklist.pushBack(func);
  }
  while (!worklist.isEmpty()) {
    Function* func = worklist.popFront();
    if (userFunctions.count(func)) {
      continue;
    }
    userFunctions.insert(func);
    for (Function* caller : func->getCallers()) {
      worklist.pushBack(caller);
    }
  }

  // If a function uses a global variable multiple times and does not call other
  // functions using this variable, convert the global variable to a local
  // variable within the function. At the beginning of the function, load the
  // value of the global variable, and at the end of the function, store it back
  // to the global variable.
  for (auto& [func, instrs] : funcToInstrs) {
    // TODO: find the best threshold
    if (instrs.size() <= 2) continue;
    // Need function prop analysis
    bool flag = false;
    for (Function* callee : func->getCallees()) {
      if (userFunctions.count(callee)) {
        flag = true;
        break;
      }
    }
    if (flag) continue;
    AllocaInst* lvAlloca =
        new AllocaInst(initValue->getType(), gv->getName() + ".lv");

    // replace use in func
    bool hasStore = false;
    bool hasLoad = false;
    for (auto& instr : instrs) {
      if (instr->isa(VT_LOAD)) {
        instr->replaceRValueAt(0, lvAlloca);
        hasLoad = true;
      } else if (instr->isa(VT_STORE)) {
        instr->replaceRValueAt(1, lvAlloca);
        hasStore = true;
      } else {
        assert(0);
      }
    }
    // load at entry
    if (hasLoad) {
      LoadInst* loadFromGv = new LoadInst(gv, gv->getName());
      StoreInst* storeToLv = new StoreInst(loadFromGv, lvAlloca);
      BasicBlock* entry = func->getEntry();
      entry->pushInstrAtHead(storeToLv);
      entry->pushInstrAtHead(loadFromGv);
      entry->pushInstrAtHead(lvAlloca);
    } else {
      StoreInst* storeToLv = new StoreInst(initValue, lvAlloca);
      BasicBlock* entry = func->getEntry();
      entry->pushInstrAtHead(storeToLv);
      entry->pushInstrAtHead(lvAlloca);
    }
    // store before return
    if (hasStore) {
      for (BasicBlock* block : *func->getBasicBlocks()) {
        Instruction* tail = block->getTailInstr();
        if (!tail || !tail->isa(VT_RET)) continue;
        LoadInst* loadFromLv = new LoadInst(lvAlloca, gv->getName() + ".lb");
        StoreInst* storeToGv = new StoreInst(loadFromLv, gv);
        loadFromLv->moveBefore(tail);
        storeToGv->moveBefore(tail);
      }
    }
    changedFunctions.insert(func);
    changed = true;
  }
  return changed;
}
