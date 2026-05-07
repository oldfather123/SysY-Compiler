#include "FunctionPropAnalysis.hh"

bool FunctionPropAnalysis::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  LinkedList<Function*> workList;
  for (Function* func : *module->getFunctions()) {
    func->clearCall();
    workList.pushBack(func);
  }

  // A function level worklist
  while (!workList.isEmpty()) {
    Function* function = workList.popFront();
    if (function->isExtern()) continue;
    if (runOnFunction(function)) {
      changed = true;
      for (Function* caller : function->getCallers()) {
        workList.pushBack(caller);
      }
    }
  }
  return changed;
}

// TODO: "Is '1 / 0' considered a test case?
// Division by zero triggers an interrupt and also has side effects.
bool FunctionPropAnalysis::runOnFunction(Function* func) {
  bool changed = false;
  size_t size = 0;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    for (Instruction* instr : *block->getInstructions()) {
      size++;
      if (instr->isa(VT_CALL)) {
        CallInst* callInst = dynamic_cast<CallInst*>(instr);
        Function* callee = callInst->getFunction();
        if (!func->isRecursive() && callee == func) {
          func->setRecursive(true);
          changed = true;
        }
        if (!func->hasSideEffects() && callee->hasSideEffects()) {
          func->setSideEffects(true);
          changed = true;
        }
        if (func->isPureFunction() && !callee->isPureFunction()) {
          func->setPureFunction(false);
          changed = true;
        }
        if (!func->hasMemRead() && callee->hasMemRead()) {
          func->setMemRead(true);
          changed = true;
        }
        if (!func->hasMemWrite() && callee->hasMemWrite()) {
          func->setMemWrite(true);
          changed = true;
        }
        Function::addCall(func, callee);
      } else if (instr->isa(VT_STORE)) {
        /**
         * FIXME: After the mem2reg pass, local variables, except for arrays,
         * are represented using registers and do not involve stores. Therefore,
         * there are only two possible destinations for stores: global variables
         * or local arrays. Storing to global variables causes side effects,
         * whereas local arrays do not. Since there is currently no analysis to
         * determine whether an address is global or local, all stores are
         * considered to have side effects.
         */
        if (!func->hasSideEffects()) {
          func->setSideEffects(true);
          changed = true;
        }
        if (func->isPureFunction()) {
          func->setPureFunction(false);
          changed = true;
        }
        if (!func->hasMemWrite()) {
          func->setMemWrite(true);
          changed = true;
        }
      } else if (instr->isa(VT_LOAD)) {
        /**
         * FIXME: Same problem like load
         */
        if (func->isPureFunction()) {
          func->setPureFunction(false);
          changed = true;
        }
        if (!func->hasMemRead()) {
          func->setMemRead(true);
          changed = true;
        }
      }
    }
  }
  func->setSize(size);
  return changed;
}