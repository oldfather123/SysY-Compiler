#include "Inlining.hh"

#include <functional>

bool Inlining::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  unordered_set<Function*> visited;
  vector<Function*> sortlist;
  Function* mainFunc = 0;
  for (Function* func : *module->getFunctions()) {
    if (func->getName() == "main") {
      mainFunc = func;
      break;
    }
  }
  assert(mainFunc);
  std::function<void(Function*)> dfs = [&](Function* func) {
    if (visited.count(func)) return;
    visited.insert(func);
    sortlist.push_back(func);
    for (Function* callee : func->getCallees()) {
      dfs(callee);
    }
    return;
  };
  dfs(mainFunc);

  for (auto it = sortlist.rbegin(); it != sortlist.rend(); it++) {
    changed |= runOnFunction(*it);
  }
  return changed;
}

bool Inlining::runOnFunction(Function* func) {
  bool changed = false;
  BBListPtr blockList = func->getBasicBlocks();
  for (auto blockIter = blockList->begin(); blockIter != blockList->end();
       ++blockIter) {
    BasicBlock* block = *blockIter;
    LinkedList<Instruction*>* instructions = block->getInstructions();
    for (auto instrIter = instructions->begin();
         instrIter != instructions->end();) {
      Instruction* instr = *instrIter;
      if (!instr->isa(VT_CALL)) {
        ++instrIter;
        continue;
      }

      CallInst* callInst = dynamic_cast<CallInst*>(instr);
      Function* callee = callInst->getFunction();
      if (callee == func || !needInline(callee)) {
        ++instrIter;
        continue;
      }

      changed = true;

      unordered_map<Value*, Value*> replaceMap;         // old -> new
      unordered_map<Value*, Value*> reverseReplaceMap;  // new -> old
      unordered_map<BasicBlock*, BasicBlock*> blockMap;

      BasicBlock* splitBlock = block->split(instrIter);
      ++instrIter;

      // modify phi from block
      vector<Use*> changedUsers;
      for (Use* use = block->getUseHead(); use; use = use->next) {
        Instruction* userInst = use->instr;
        if (!userInst->isa(VT_PHI)) continue;
        changedUsers.push_back(use);
      }
      for (Use* use : changedUsers) {
        use->replaceValue(splitBlock);
      }

      FuncType* funcType = dynamic_cast<FuncType*>(callee->getType());
      int argSize = funcType->getArgSize();
      assert(argSize == callInst->getRValueSize());
      for (int i = 0; i < argSize; i++) {
        replaceMap.emplace(funcType->getArgument(i), callInst->getRValue(i));
      }

      for (BasicBlock* oldBlock : *callee->getBasicBlocks()) {
        BasicBlock* newBlock = oldBlock->clone(replaceMap);
        newBlock->setParent(func);
        blockMap.emplace(oldBlock, newBlock);
        blockList->insertAfter(blockIter, newBlock);
        ++blockIter;
        replaceMap.emplace(oldBlock, newBlock);
      }

      blockList->insertAfter(blockIter, splitBlock);

      // deal with exit
      BasicBlock* newExit = blockMap[callee->getExit()];
      PhiInst* retPhi = 0;
      if (callInst->getType()->getTypeTag() != TT_VOID) {
        retPhi = new PhiInst(callInst->getName() + "_phi");
        newExit->pushInstr(retPhi);
        callInst->replaceAllUsesWith(retPhi);
      }
      newExit->pushInstr(new JumpInst(splitBlock));

      // deal with entry
      callInst->eraseFromParent();
      callInst->deleteUseList();
      delete callInst;
      block->pushInstr(new JumpInst(blockMap[callee->getEntry()]));

      for (auto [oldValue, newValue] : replaceMap) {
        reverseReplaceMap.emplace(newValue, oldValue);
      }

      for (auto [oBlock, nBlock] : blockMap) {
        LinkedList<Instruction*>* nInstrList = nBlock->getInstructions();
        for (auto it = nInstrList->begin(); it != nInstrList->end();) {
          Instruction* nInstr = *it;
          Instruction* oInstr =
              dynamic_cast<Instruction*>(reverseReplaceMap[nInstr]);
          if (!oInstr) {
            ++it;
            continue;
          }
          nInstr->cloneUseList(replaceMap, oInstr->getUseList());

          // deal with return value
          if (nInstr->isa(VT_RET)) {
            ReturnInst* retInstr = dynamic_cast<ReturnInst*>(nInstr);
            if (retPhi) {
              retPhi->pushIncoming(retInstr->getRValue(0), nBlock);
            }
            ++it;
            nInstr->eraseFromParent();
            nInstr->deleteUseList();
            delete nInstr;
            nBlock->pushInstr(new JumpInst(newExit));

          } else if (nInstr->isa(VT_ALLOCA)) {
            // move alloca to entry block
            ++it;
            nInstr->eraseFromParent();
            func->getEntry()->pushInstrAtHead(nInstr);

          } else {
            ++it;
          }
        }
      }
    }
  }
  if (changed) {
    func->resetCFG();
    func->resetDT();
  }
  return changed;
}

bool Inlining::needInline(Function* func) {
  return !func->isExtern() && !func->isRecursive() &&
         func->getLineSize() < INLINE_THRESHOLD;
}
