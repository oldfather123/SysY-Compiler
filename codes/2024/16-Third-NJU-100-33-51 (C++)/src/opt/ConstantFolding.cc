#include "ConstantFolding.hh"

bool ConstantFolding::runOnModule(ANTPIE::Module *module) {
  bool changed = false;
  for (Function *function : *module->getFunctions()) {
    runOnFunction(function);
  }
  return changed;
}

bool ConstantFolding::runOnFunction(Function *func) {
  bool changed = false;
  DomTree *dt = func->getDT();
  if (!dt) dt = func->buildDT();
  return constantFoldingDFS(func->getEntry(), dt);
}

bool ConstantFolding::constantFoldingDFS(BasicBlock *bb, DomTree *dt) {
  bool changed = false;
  LinkedList<Instruction *> *instrList = bb->getInstructions();
  for (auto it = instrList->begin(); it != instrList->end();) {
    Instruction *instr = *it;
    ++it;
    Value *newValue = instructionSimplify(instr);
    if (newValue) {
      changed = true;
      instr->replaceAllUsesWith(newValue);
      instr->eraseFromParent();
      instr->deleteUseList();
      delete instr;
    }
  }

  for (BasicBlock *succBB : *dt->getDomChildren(bb)) {
    changed |= constantFoldingDFS(succBB, dt);
  }
  return changed;
}

Value *ConstantFolding::instructionSimplify(Instruction *instr) {
  ValueTag vt = instr->getValueTag();
  if (!(vt == VT_BOP || vt == VT_ICMP || vt == VT_FCMP || vt == VT_FPTOSI ||
        vt == VT_SITOFP || vt == VT_ZEXT)) {
    return nullptr;
  }
  if (!constantOperand(instr)) return nullptr;
  switch (instr->getValueTag()) {
    case VT_BOP:
      return simplifyBOP((BinaryOpInst *)instr);
    case VT_ICMP:
      return simplifyICMP((IcmpInst *)instr);
    case VT_FCMP:
      return simplifyFCMP((FcmpInst *)instr);
    case VT_FPTOSI:
      return simplifyFPTOSI((FptosiInst *)instr);
    case VT_SITOFP:
      return simplifySITOFP((SitofpInst *)instr);
    case VT_ZEXT:
      return simplifyZEXT((ZextInst *)instr);
    default:
      return nullptr;
  }
}

// Return true if all operands are constant
bool ConstantFolding::constantOperand(Instruction *instr) {
  int rValSize = instr->getRValueSize();
  for (int i = 0; i < rValSize; i++) {
    if (!dynamic_cast<Constant *>(instr->getRValue(i))) return false;
  }
  return true;
}

Value *ConstantFolding::simplifyBOP(BinaryOpInst *instr) {
  Value *newValue = 0;
  switch (instr->getOpTag()) {
    case ADD: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs + rhs);
      break;
    }
    case FADD: {
      float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
      float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
      newValue = FloatConstant::getConstFloat(lhs + rhs);
      break;
    }
    case SUB: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs - rhs);
      break;
    }
    case FSUB: {
      float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
      float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
      newValue = FloatConstant::getConstFloat(lhs - rhs);
      break;
    }
    case MUL: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs * rhs);
      break;
    }
    case FMUL: {
      float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
      float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
      newValue = FloatConstant::getConstFloat(lhs * rhs);
      break;
    }
    case SDIV: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs / rhs);
      break;
    }
    case FDIV: {
      float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
      float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
      newValue = FloatConstant::getConstFloat(lhs / rhs);
      break;
    }
    case SREM: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs % rhs);
      break;
    }
    case FREM: {
      float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
      float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
      newValue = FloatConstant::getConstFloat(std::fmod(lhs, rhs));
      break;
    }
    case AND: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs & rhs);
      break;
    }
    case OR: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs | rhs);
      break;
    }
    case XOR: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs ^ rhs);
      break;
    }
    case LSHR: {
      unsigned int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs >> rhs);
      break;
    }
    case ASHR: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt(lhs >> rhs);
      break;
    }
    case SHL: {
      int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
      int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
      newValue = IntegerConstant::getConstInt((long long)lhs << rhs);
      break;
    }
    default:
      assert(0);
  }
  return newValue;
}
Value *ConstantFolding::simplifyICMP(IcmpInst *instr) {
  int lhs = ((IntegerConstant *)instr->getRValue(0))->getValue();
  int rhs = ((IntegerConstant *)instr->getRValue(1))->getValue();
  Value *newValue = 0;
  switch (instr->getOpTag()) {
    case EQ: {
      newValue = BoolConstant::getConstBool(lhs == rhs);
      break;
    }
    case NE: {
      newValue = BoolConstant::getConstBool(lhs != rhs);
      break;
    }
    case SLE: {
      newValue = BoolConstant::getConstBool(lhs <= rhs);
      break;
    }
    case SGE: {
      newValue = BoolConstant::getConstBool(lhs >= rhs);
      break;
    }
    case SLT: {
      newValue = BoolConstant::getConstBool(lhs < rhs);
      break;
    }
    case SGT: {
      newValue = BoolConstant::getConstBool(lhs > rhs);
      break;
    }
    default:
      assert(0);
  }
  return newValue;
}
Value *ConstantFolding::simplifyFCMP(FcmpInst *instr) {
  float lhs = ((FloatConstant *)instr->getRValue(0))->getValue();
  float rhs = ((FloatConstant *)instr->getRValue(1))->getValue();
  Value *newValue = 0;
  switch (instr->getOpTag()) {
    case OEQ: {
      newValue = BoolConstant::getConstBool(lhs == rhs);
      break;
    }
    case ONE: {
      newValue = BoolConstant::getConstBool(lhs != rhs);
      break;
    }
    case OLE: {
      newValue = BoolConstant::getConstBool(lhs <= rhs);
      break;
    }
    case OGE: {
      newValue = BoolConstant::getConstBool(lhs >= rhs);
      break;
    }
    case OLT: {
      newValue = BoolConstant::getConstBool(lhs < rhs);
      break;
    }
    case OGT: {
      newValue = BoolConstant::getConstBool(lhs > rhs);
      break;
    }
    default:
      assert(0);
  }
  return newValue;
}

Value *ConstantFolding::simplifySITOFP(SitofpInst *instr) {
  int intValue = ((IntegerConstant *)instr->getRValue(0))->getValue();
  return FloatConstant::getConstFloat(intValue);
}

Value *ConstantFolding::simplifyFPTOSI(FptosiInst *instr) {
  float floatValue = ((FloatConstant *)instr->getRValue(0))->getValue();
  return IntegerConstant::getConstInt(floatValue);
}

Value *ConstantFolding::simplifyZEXT(ZextInst *instr) {
  Type *targetType = instr->getType();
  if (targetType->getTypeTag() != TT_INT32) {
    assert(0);
  }
  BoolConstant *boolConst = dynamic_cast<BoolConstant *>(instr->getRValue(0));
  assert(boolConst);
  return IntegerConstant::getConstInt(boolConst->getValue());
}
