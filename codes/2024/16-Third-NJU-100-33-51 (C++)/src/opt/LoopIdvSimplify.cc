#include "LoopIdvSimplify.hh"

bool LoopIdvSimplify::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }
  return changed;
}

bool LoopIdvSimplify::runOnFunction(Function* func) {
  bool changed = false;
  for (LoopInfo* loopInfo : func->getLoopInfoBase()->loopInfos) {
    changed |= runOnLoop(loopInfo);
  }
  return changed;
}

/**
 * while(i < n) {
 *  a = i * 4;
 *  i = i + 1;
 * }
 * =>
 * -----------------------
 * while(i < n) {
 *  a = a + 4;
 *  i = i + 1;
 * }
 */
bool LoopIdvSimplify::runOnLoop(LoopInfo* loopInfo) {
  bool changed = false;
  SimpleLoopInfo* simpleLoop = loopInfo->simpleLoop;
  if (!simpleLoop) return false;
  PhiInst* counter = simpleLoop->phiInstr;
  BinaryOpInst* strideInst = simpleLoop->strideInstr;
  if (strideInst->getOpTag() != ADD) return false;
  Value* stride = strideInst->getRValue(1);
  if (!loopInfo->isInvariant(stride)) return false;

  BranchInst* branch = simpleLoop->brInstr;
  BasicBlock* exitBlock = *loopInfo->exits.begin();
  BasicBlock* trueBlock = nullptr;
  BasicBlock* header = loopInfo->header;
  BasicBlock* preHeader = loopInfo->preHeader;
  Value* initValue = simpleLoop->initValue;
  if ((BasicBlock*)branch->getRValue(1) == exitBlock) {
    trueBlock = (BasicBlock*)branch->getRValue(2);
  } else {
    trueBlock = (BasicBlock*)branch->getRValue(1);
  }
  for (Instruction* instr : *trueBlock->getInstructions()) {
    BinaryOpInst* bopInstr = dynamic_cast<BinaryOpInst*>(instr);
    if (!bopInstr || bopInstr == strideInst) continue;
    if (bopInstr->getRValue(1) == counter &&
        (bopInstr->getOpTag() == ADD || bopInstr->getOpTag() == MUL)) {
      bopInstr->swapRValueAt(0, 1);
    }
    if (bopInstr->getRValue(0) != counter) continue;
    Value* offset = bopInstr->getRValue(1);
    if (!loopInfo->isInvariant(offset)) continue;
    switch (bopInstr->getOpTag()) {
      case ADD: {
        PhiInst* idvPhi = new PhiInst(bopInstr->getName() + ".phi");
        BinaryOpInst* idvInit = new BinaryOpInst(ADD, initValue, offset,
                                                 bopInstr->getName() + ".init");
        idvInit->moveBefore(preHeader->getTailInstr());
        idvPhi->pushIncoming(idvInit, preHeader);
        BinaryOpInst* idvStrideInst = new BinaryOpInst(
            ADD, idvPhi, stride, bopInstr->getName() + ".stride");
        idvStrideInst->moveBefore(trueBlock->getTailInstr());
        bopInstr->replaceAllUsesWith(idvPhi);
        for (BasicBlock* latch : loopInfo->latches) {
          idvPhi->pushIncoming(idvStrideInst, latch);
        }
        header->pushInstrAtHead(idvPhi);
        changed = true;
        break;
      }

      case MUL: {
        PhiInst* idvPhi = new PhiInst(bopInstr->getName() + ".phi");
        BinaryOpInst* idvInit = new BinaryOpInst(MUL, initValue, offset,
                                                 bopInstr->getName() + ".init");
        BinaryOpInst* increValue = new BinaryOpInst(
            MUL, stride, offset, bopInstr->getName() + ".incr");
        idvInit->moveBefore(preHeader->getTailInstr());
        increValue->moveBefore(preHeader->getTailInstr());
        idvPhi->pushIncoming(idvInit, preHeader);
        BinaryOpInst* idvStrideInst = new BinaryOpInst(
            ADD, idvPhi, increValue, bopInstr->getName() + ".stride");
        idvStrideInst->moveBefore(trueBlock->getTailInstr());
        bopInstr->replaceAllUsesWith(idvPhi);
        for (BasicBlock* latch : loopInfo->latches) {
          idvPhi->pushIncoming(idvStrideInst, latch);
        }
        header->pushInstrAtHead(idvPhi);
        changed = true;
        break;
      }

      case SHL: {
        PhiInst* idvPhi = new PhiInst(bopInstr->getName() + ".phi");
        BinaryOpInst* idvInit = new BinaryOpInst(SHL, initValue, offset,
                                                 bopInstr->getName() + ".init");
        Value* mulValue = 0;
        if (offset->isa(VT_INTCONST)) {
          mulValue = IntegerConstant::getConstInt(
              1 << ((IntegerConstant*)offset)->getValue());
        } else {
          mulValue = new BinaryOpInst(SHL, IntegerConstant::getConstInt(1),
                                      offset, bopInstr->getName() + ".mulv");
        }
        BinaryOpInst* increValue = new BinaryOpInst(
            MUL, stride, mulValue, bopInstr->getName() + ".incr");
        idvInit->moveBefore(preHeader->getTailInstr());
        increValue->moveBefore(preHeader->getTailInstr());
        idvPhi->pushIncoming(idvInit, preHeader);
        BinaryOpInst* idvStrideInst = new BinaryOpInst(
            ADD, idvPhi, increValue, bopInstr->getName() + ".stride");
        idvStrideInst->moveBefore(trueBlock->getTailInstr());
        bopInstr->replaceAllUsesWith(idvPhi);
        for (BasicBlock* latch : loopInfo->latches) {
          idvPhi->pushIncoming(idvStrideInst, latch);
        }
        header->pushInstrAtHead(idvPhi);
        changed = true;
        break;
      }
      default:
        continue;
    }
  }
  return changed;
}
