// Reference: https://www.zzzconsulting.se/download/OurMem2Reg.cpp

#include "MemToReg.hh"

#include "DomTree.hh"

int id = 0;

struct ValueInfo {
  AllocaInst* allocaInstr;
  LinkedList<BasicBlock*> defBlocks;
  LinkedList<Value*> defStask;
  ValueInfo(AllocaInst* alloca_) : allocaInstr(alloca_) {}
};

bool isAllocaPromotable(AllocaInst* alloc) {
  for (Use* use = alloc->getUseHead(); use; use = use->next) {
    if (use->instr->isa(VT_GEP)) {
      return false;
    }
  }
  return true;
}

void MemToReg::runPass() {
  for (Instruction* instr : *function->getEntry()->getInstructions()) {
    AllocaInst* allocaInstr;
    if ((allocaInstr = dynamic_cast<AllocaInst*>(instr)) &&
        isAllocaPromotable(allocaInstr)) {
      ValueInfo* valueInfo = new ValueInfo(allocaInstr);
      if (linkDefsAndUsesToVar(valueInfo)) {
        valueInfos.pushBack(valueInfo);
      } else {
        delete valueInfo;
      }
      trashList.pushBack(allocaInstr);
    }
  }

  for (ValueInfo* valueInfo : valueInfos) {
    BBListPtr phiBlocks = new LinkedList<BasicBlock*>();
    function->getDT()->calculateIDF(&valueInfo->defBlocks, phiBlocks);
    Type* phiType = valueInfo->allocaInstr->getElemType();
    for (BasicBlock* bb : *phiBlocks) {
      PhiInst* phiInstr = new PhiInst(phiType, "phi");
      bb->pushInstrAtHead(phiInstr);
      instToValueInfo[phiInstr] = valueInfo;
    }
    delete phiBlocks;
  }
  renameRecursive(function->getEntry());

  while (!trashList.isEmpty()) {
    Instruction* trash = trashList.popFront();
    trash->eraseFromParent();
    trash->deleteUseList();
    delete trash;
  }
}

bool MemToReg::linkDefsAndUsesToVar(ValueInfo* valueInfo) {
  for (Use* use = valueInfo->allocaInstr->getUseHead(); use; use = use->next) {
    Instruction* useInstr;
    if (useInstr = dynamic_cast<LoadInst*>(use->instr)) {
      instToValueInfo[useInstr] = valueInfo;
    } else if ((useInstr = dynamic_cast<StoreInst*>(use->instr))) {
      if (useInstr->getRValue(1) == valueInfo->allocaInstr) {
        instToValueInfo[useInstr] = valueInfo;
        valueInfo->defBlocks.pushBack(useInstr->getParent());
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

void MemToReg::renameRecursive(BasicBlock* bb) {
  for (Instruction* instr : *bb->getInstructions()) {
    ValueInfo* valueInfo;
    if (instr->isa(VT_STORE) && (valueInfo = instToValueInfo[instr])) {
      valueInfo->defStask.pushFront(instr->getRValue(0));
    } else if (instr->isa(VT_LOAD) && (valueInfo = instToValueInfo[instr])) {
      if (!valueInfo->defStask.isEmpty()) {
        instr->replaceAllUsesWith(valueInfo->defStask.front());
      }
    } else if (instr->isa(VT_PHI) && (valueInfo = instToValueInfo[instr])) {
      valueInfo->defStask.pushFront(instr);
    }
  }

  for (BasicBlock* succ : *function->getCFG()->getSuccOf(bb)) {
    for (Instruction* instr : *succ->getInstructions()) {
      PhiInst* phi;
      ValueInfo* valueInfo;
      if ((phi = dynamic_cast<PhiInst*>(instr)) &&
          (valueInfo = instToValueInfo[instr]))
        if (!valueInfo->defStask.isEmpty()) {
          phi->pushIncoming(valueInfo->defStask.front(), bb);
        } else {
          phi->pushIncoming(phi->getType()->getZeroInit(), bb);
        }
    }
  }

  for (BasicBlock* domChild : *function->getDT()->getDomChildren(bb)) {
    renameRecursive(domChild);
  }

  for (Instruction* instr : *bb->getInstructions()) {
    ValueInfo* valueInfo;
    if (instr->isa(VT_STORE) && (valueInfo = instToValueInfo[instr])) {
      valueInfo->defStask.popFront();
      trashList.pushFront(instr);
    } else if (instr->isa(VT_PHI) && (valueInfo = instToValueInfo[instr])) {
      valueInfo->defStask.popFront();
    } else if (instr->isa(VT_LOAD) && (valueInfo = instToValueInfo[instr])) {
      trashList.pushFront(instr);
    }
  }
}

bool MemToReg::runOnModule(ANTPIE::Module* module) {
  for (Function* func : *module->getFunctions()) {
    runOnFunction(func);
  }
  return true;
}

bool MemToReg::runOnFunction(Function* func) {
  instToValueInfo.clear();
  valueInfos.clear();
  if (!func->getCFG()) func->buildCFG();
  if (!func->getDT()) func->buildDT();
  if (!func->getDT()->dfReady()) func->getDT()->calculateDF();
  function = func;
  runPass();
  return true;
}