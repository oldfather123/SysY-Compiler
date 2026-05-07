#include "StrengthReduction.hh"

#include <cmath>

bool isPowerOfTwo(int x) { return x > 0 && (x & (x - 1)) == 0; }

bool StrengthReduction::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool StrengthReduction::runOnFunction(Function* func) {
  bool changed = false;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    auto instrList = block->getInstructions();
    for (auto it = instrList->begin(); it != instrList->end();) {
      Instruction* instr = *it;
      ++it;
      BinaryOpInst* bopInstr = dynamic_cast<BinaryOpInst*>(instr);
      if (!bopInstr) continue;
      switch (bopInstr->getOpTag()) {
        case ADD: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // x + 0 => x
          if (rhs->isa(VT_INTCONST) &&
              ((IntegerConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          // 0 + x => x
          if (lhs->isa(VT_INTCONST) &&
              ((IntegerConstant*)lhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(rhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          // x + x => x << 1
          if (lhs == rhs) {
            bopInstr->setOpTag(SHL);
            bopInstr->replaceRValueAt(1, IntegerConstant::getConstInt(1));
            changed = true;
            continue;
          }
          break;
        }
        case FADD: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // x + 0 => x
          if (rhs->isa(VT_FLOATCONST) &&
              ((FloatConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          // 0 + x => x
          if (lhs->isa(VT_FLOATCONST) &&
              ((FloatConstant*)lhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(rhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          break;
        }
        case SUB: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // x - 0 => x
          if (rhs->isa(VT_INTCONST) &&
              ((IntegerConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          break;
        }
        case FSUB: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // x - 0 => x
          if (rhs->isa(VT_FLOATCONST) &&
              ((FloatConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          }
          break;
        }
        case MUL: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);

          if (lhs->isa(VT_INTCONST)) {
            std::swap(lhs, rhs);
            bopInstr->swapRValueAt(0, 1);
          }
          if (!rhs->isa(VT_INTCONST)) continue;
          int rhsInt = ((IntegerConstant*)rhs)->getValue();
          if (rhsInt == 0) {
            // x * 0 => 0
            bopInstr->replaceAllUsesWith(rhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          } else if (rhsInt == 1) {
            // x * 1 => x
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          } else if (isPowerOfTwo(rhsInt)) {
            // x * 2^n => x << n
            bopInstr->setOpTag(SHL);
            bopInstr->replaceRValueAt(
                1, IntegerConstant::getConstInt(std::log2(rhsInt)));
            changed = true;
            continue;
          }
        }
        case FMUL: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);

          if (lhs->isa(VT_FLOATCONST)) {
            std::swap(lhs, rhs);
            bopInstr->swapRValueAt(0, 1);
          }
          if (!rhs->isa(VT_FLOATCONST)) continue;
          float rhsFloat = ((FloatConstant*)rhs)->getValue();
          if (rhsFloat == 0) {
            // x * 0 => 0
            bopInstr->replaceAllUsesWith(rhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
          } else if (rhsFloat == 1) {
            // x * 1 => x
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          } else if (rhsFloat == 2) {
            // x * 2 => x + x
            bopInstr->setOpTag(FADD);
            bopInstr->replaceRValueAt(1, lhs);
            changed = true;
            continue;
          }
        }
        case SDIV: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);

          if (!rhs->isa(VT_INTCONST)) continue;
          int rhsInt = ((IntegerConstant*)rhs)->getValue();
          if (rhsInt == 1) {
            // x / 1 => x
            bopInstr->replaceAllUsesWith(lhs);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            changed = true;
            continue;
          } else if (rhsInt == 2) {
            // x / 2 => (x - (x >> 31)) >> 1
            BinaryOpInst* shr1 = new BinaryOpInst(
                ASHR, lhs, IntegerConstant::getConstInt(31), "shr");
            BinaryOpInst* sub = new BinaryOpInst(SUB, lhs, shr1, "sub");
            BinaryOpInst* shr2 = new BinaryOpInst(
                ASHR, sub, IntegerConstant::getConstInt(1), "shr");
            bopInstr->replaceAllUsesWith(shr2);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            block->pushInstrBefore(shr1, it);
            block->pushInstrBefore(sub, it);
            block->pushInstrBefore(shr2, it);
            changed = true;
            continue;
          } else if (isPowerOfTwo(rhsInt)) {
            // x / 2^n =>
            // (x +
            //    (x >> 31) & (1 << n - 1)
            // ) >> n
            int n = std::log2(rhsInt);
            BinaryOpInst* shr1 = new BinaryOpInst(
                ASHR, lhs, IntegerConstant::getConstInt(31), "shr");
            int mask = rhsInt - 1;
            BinaryOpInst* andInst = new BinaryOpInst(
                AND, shr1, IntegerConstant::getConstInt(rhsInt - 1), "and");
            BinaryOpInst* add = new BinaryOpInst(ADD, lhs, andInst, "add");
            BinaryOpInst* shr2 = new BinaryOpInst(
                ASHR, add, IntegerConstant::getConstInt(n), "shr");
            bopInstr->replaceAllUsesWith(shr2);
            bopInstr->eraseFromParent();
            bopInstr->deleteUseList();
            block->pushInstrBefore(shr1, it);
            block->pushInstrBefore(andInst, it);
            block->pushInstrBefore(add, it);
            block->pushInstrBefore(shr2, it);
            changed = true;
            continue;
          }
          break;
        }
        case SREM: {
          continue;
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          if (!rhs->isa(VT_INTCONST)) continue;
          int rhsInt = ((IntegerConstant*)rhs)->getValue();
          if (!isPowerOfTwo(rhsInt)) continue;
          // x % 2^n => x & (2^n - 1)
          bopInstr->setOpTag(AND);
          bopInstr->replaceRValueAt(1,
                                    IntegerConstant::getConstInt(rhsInt - 1));
          changed = true;
          continue;
        }
        case AND: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // 0 & x (x & 0) => 0
          if (lhs->isa(VT_INTCONST) &&
              ((IntegerConstant*)lhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(IntegerConstant::getConstInt(0));
          } else if (rhs->isa(VT_INTCONST) &&
                     ((IntegerConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(IntegerConstant::getConstInt(0));
          }
          changed = true;
          continue;
        }
        case OR: {
          Value* lhs = bopInstr->getRValue(0);
          Value* rhs = bopInstr->getRValue(1);
          // 0 | x (x | 0) => x
          if (lhs->isa(VT_INTCONST) &&
              ((IntegerConstant*)lhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(rhs);
          } else if (rhs->isa(VT_INTCONST) &&
                     ((IntegerConstant*)rhs)->getValue() == 0) {
            bopInstr->replaceAllUsesWith(lhs);
          }
          changed = true;
          continue;
        }
        default:
          break;
      }
    }
  }
  return changed;
}