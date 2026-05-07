#include "Instruction.hh"

#include <cassert>

#include "Function.hh"
#include "Type.hh"

/******************** Utils Function ******************************/

void Instruction::pushValue(Value* v) {
  Use* use = new Use(this, v);
  useList->push_back(use);
  v->addUser(use);
}

Value* Instruction::getRValue(int idx) const {
  assert(idx < useList->size());
  return useList->at(idx)->value;
}

void Instruction::eraseFromParent() {
  if (block->getTailInstr() == this) {
    block->setTailInstr(nullptr);
  }
  block->getInstructions()->remove(this);
  block = nullptr;
}

void Instruction::deleteUseList() {
  for (Use* use : *useList) {
    use->removeFromValue();
    delete use;
  }
  useList->clear();
  if (isa(VT_CALL)) {
    CallInst* callInst = (CallInst*)this;
    callInst->getFunction()->deleteCallSite(callInst);
  }
}

bool BranchInst::replaceDestinationWith(BasicBlock* oldBlock,
                                        BasicBlock* newBlock) {
  int n = getRValueSize();
  for (int i = 1; i < n; i++) {
    BasicBlock* expectOldBB = dynamic_cast<BasicBlock*>(getRValue(i));
    if (expectOldBB == oldBlock) {
      Use* use = useList->at(i);
      use->removeFromValue();
      use->value = newBlock;
      newBlock->addUser(use);

      // Modify cfg
      BasicBlock* block = getParent();
      CFG* cfg = block->getParent()->getCFG();
      if (cfg) {
        cfg->eraseEdge(block, oldBlock);
        cfg->addEdge(block, newBlock);
      }
      return true;
    }
  }
  return false;
}

void Instruction::replaceRValueAt(int idx, Value* newValue) {
  Use* use = useList->at(idx);
  use->removeFromValue();
  use->value = newValue;
  newValue->addUser(use);
}

bool JumpInst::replaceDestinationWith(BasicBlock* oldBlock,
                                      BasicBlock* newBlock) {
  if (oldBlock != dynamic_cast<BasicBlock*>(getRValue(0))) return false;
  Use* use = useList->at(0);
  use->removeFromValue();
  use->value = newBlock;
  newBlock->addUser(use);

  // Modify cfg
  BasicBlock* block = getParent();
  CFG* cfg = block->getParent()->getCFG();
  if (cfg) {
    cfg->eraseEdge(block, oldBlock);
    cfg->addEdge(block, newBlock);
  }
  return true;
}

void Instruction::cloneUseList(unordered_map<Value*, Value*>& replaceMap,
                               vector<Use*>* fromUseList) {
  for (Use* use : *fromUseList) {
    Value* value = use->value;
    auto it = replaceMap.find(value);
    if (it != replaceMap.end()) {
      value = it->second;
    }
    pushValue(value);
  }
}

bool Instruction::deleteRValueAt(int idx) {
  Use* use = useList->at(idx);
  use->removeFromValue();
  useList->erase(useList->begin() + idx);
  return true;
}

bool Instruction::mayWriteToMemory() {
  if (isa(VT_STORE)) {
    return true;
  }
  if (CallInst* callInst = dynamic_cast<CallInst*>(this)) {
    if (callInst->getFunction()->hasMemWrite()) {
      return true;
    }
  }
  return false;
}

bool Instruction::mayReadFromMemory() {
  if (isa(VT_LOAD)) {
    return true;
  }
  if (CallInst* callInst = dynamic_cast<CallInst*>(this)) {
    if (callInst->getFunction()->hasMemRead()) {
      return true;
    }
  }
  return false;
}

Value* PhiInst::deleteIncomingFrom(BasicBlock* block) {
  int icSize = getRValueSize() / 2;
  int loc = -1;
  for (int i = 0; i < icSize; i++) {
    if (getRValue(i * 2 + 1) == (Value*)block) {
      loc = i;
      break;
    }
  }

  if (loc == -1) return nullptr;
  Value* oldValue = getRValue(2 * loc);
  deleteRValueAt(2 * loc + 1);
  deleteRValueAt(2 * loc);

  return oldValue;
}

void Instruction::moveBefore(Instruction* instr) {
  if (getParent()) {
    eraseFromParent();
  }
  BasicBlock* destBlock = instr->getParent();
  setParent(destBlock);
  destBlock->getInstructions()->insertBefore(instr, this);
}

void Instruction::swapRValueAt(int idx1, int idx2) {
  assert(idx1 < useList->size() && idx2 < useList->size());
  auto tmp = useList->at(idx1);
  useList->at(idx1) = useList->at(idx2);
  useList->at(idx2) = tmp;
}

/*********************** Constructor ******************************/

Instruction::Instruction(ValueTag vTag) : Value(Type::getVoidType(), vTag) {
  useList = make_unique<vector<Use*>>();
}

Instruction::Instruction(string name, ValueTag vTag)
    : Value(Type::getVoidType(), LabelManager::getLabel(name), vTag) {
  useList = make_unique<vector<Use*>>();
}

Instruction::Instruction(Type* t, string name, ValueTag vTag)
    : Value(t, LabelManager::getLabel(name), vTag) {
  useList = make_unique<vector<Use*>>();
}

AllocaInst::AllocaInst(Type* type, string name)
    : Instruction(Type::getPointerType(type), name, VT_ALLOCA), elemType(type) {
  assert(type);
}

BinaryOpInst::BinaryOpInst(OpTag opType, Value* op1, Value* op2, string name)
    : Instruction(op1->getType(), name, VT_BOP), bOpType(opType) {
  pushValue(op1);
  pushValue(op2);
};

BranchInst::BranchInst(Value* cond, BasicBlock* trueBlock,
                       BasicBlock* falseBlock)
    : Instruction(VT_BR) {
  pushValue(cond);
  pushValue((Value*)trueBlock);
  pushValue((Value*)falseBlock);
}

CallInst::CallInst(Function* func, vector<Value*>& params, string name)
    : Instruction(name, VT_CALL), function(func) {
  setType(((FuncType*)func->getType())->getRetType());
  for (Value* param : params) {
    pushValue(param);
  }
  func->addCallSite(this);
}

CallInst::CallInst(Function* func, string name)
    : Instruction(name, VT_CALL), function(func) {
  setType(((FuncType*)func->getType())->getRetType());
  func->addCallSite(this);
}

void CallInst::pushArgument(Value* value) { pushValue(value); }

IcmpInst::IcmpInst(OpTag opType, Value* op1, Value* op2, string name)
    : Instruction(Type::getInt1Type(), name, VT_ICMP), icmpType(opType) {
  pushValue(op1);
  pushValue(op2);
}

IcmpInst::IcmpInst(OpTag opType, string name)
    : Instruction(Type::getInt1Type(), name, VT_ICMP), icmpType(opType) {}

FcmpInst::FcmpInst(OpTag opType, Value* op1, Value* op2, string name)
    : Instruction(Type::getInt1Type(), name, VT_FCMP), fcmpType(opType) {
  pushValue(op1);
  pushValue(op2);
}
FcmpInst::FcmpInst(OpTag opType, string name)
    : Instruction(Type::getInt1Type(), name, VT_FCMP), fcmpType(opType) {}

FptosiInst::FptosiInst(Value* src, string name)
    : Instruction(Type::getInt32Type(), name, VT_FPTOSI) {
  pushValue(src);
}

FptosiInst::FptosiInst(string name)
    : Instruction(Type::getInt32Type(), name, VT_FPTOSI) {}

GetElemPtrInst::GetElemPtrInst(Value* ptr, Value* idx1, Value* idx2,
                               string name)
    : Instruction(name, VT_GEP), ptrType(ptr->getType()) {
  setType(Type::getPointerType(
      ((ArrayType*)((PointerType*)ptrType)->getElemType())->getElemType()));
  pushValue(ptr);
  pushValue(idx1);
  pushValue(idx2);
}

GetElemPtrInst::GetElemPtrInst(Type* type, PointerType* ptrType_, string name)
    : Instruction(type, name, VT_GEP), ptrType(ptrType_) {}

GetElemPtrInst::GetElemPtrInst(Value* ptr, Value* idx1, string name)
    : Instruction(ptr->getType(), name, VT_GEP), ptrType(ptr->getType()) {
  pushValue(ptr);
  pushValue(idx1);
}

JumpInst::JumpInst(BasicBlock* block) : Instruction(VT_JUMP) {
  pushValue((Value*)block);
}

LoadInst::LoadInst(Value* addr, string name) : Instruction(name, VT_LOAD) {
  setType(((PointerType*)addr->getType())->getElemType());
  pushValue(addr);
}

PhiInst::PhiInst(string name) : Instruction(name, VT_PHI) {}

void PhiInst::pushIncoming(Value* v, BasicBlock* bb) {
  pushValue(v);
  pushValue((Value*)bb);
  setType(v->getType());
}

ReturnInst::ReturnInst(Value* retValue) : Instruction(VT_RET) {
  pushValue(retValue);
}

ReturnInst::ReturnInst() : Instruction(VT_RET) {}

SitofpInst::SitofpInst(Value* src, string name)
    : Instruction(Type::getFloatType(), name, VT_SITOFP) {
  pushValue(src);
}
SitofpInst::SitofpInst(string name)
    : Instruction(Type::getFloatType(), name, VT_SITOFP) {}

StoreInst::StoreInst(Value* value, Value* addr) : Instruction(VT_STORE) {
  pushValue(value);
  pushValue(addr);
}

ZextInst::ZextInst(Value* src, Type* dstType, string name)
    : Instruction(dstType, name, VT_ZEXT) {
  pushValue(src);
}

/********************* Print Function ********************************/

void AllocaInst::printIR(ostream& stream) const {
  stream << "%" << getName() << " = alloca " << elemType->toString();
}

string Instruction::getOpName(OpTag op) const {
  static string opNames[28] = {"add",  "fadd", "sub",  "fsub", "mul",  "fmul",
                               "sdiv", "fdiv", "srem", "frem", "lshr", "ashr",
                               "shl",  "and",  "or",   "xor",  "eq",   "ne",
                               "sle",  "slt",  "sge",  "sgt",  "oeq",  "one",
                               "ogt",  "oge",  "ole",  "olt"};
  return opNames[op];
}

void BinaryOpInst::printIR(ostream& stream) const {
  stream << toString() << " = " << getOpName(bOpType) << " "
         << getType()->toString() << " " << getRValue(0)->toString() << ", "
         << getRValue(1)->toString();
}

// br i1 %cmp, label %if.then, label %if.else
void BranchInst::printIR(ostream& stream) const {
  Value* cond = getRValue(0);
  stream << "br " << cond->getType()->toString() << " " << cond->toString()
         << ", label " << getRValue(1)->toString() << ", label "
         << getRValue(2)->toString();
}

// %call = call i32 @foo(i32 noundef 1, i32 noundef 2)
void CallInst::printIR(ostream& stream) const {
  Type* retType = ((FuncType*)function->getType())->getRetType();
  if (retType->getTypeTag() != TT_VOID) {
    stream << toString() << " = ";
  }
  stream << "call " << retType->toString() << " @" << function->getName()
         << "(";
  int vSize = getRValueSize();
  for (int i = 0; i < vSize; i++) {
    if (i != 0) {
      stream << ", ";
    }
    Value* arg = getRValue(i);
    stream << arg->getType()->toString() << " " << arg->toString();
  }
  stream << ")";
}

void IcmpInst::printIR(ostream& stream) const {
  stream << toString() << " = icmp " << getOpName(icmpType) << " i32 "
         << getRValue(0)->toString() << ", " << getRValue(1)->toString();
}

void FcmpInst::printIR(ostream& stream) const {
  stream << toString() << " = fcmp " << getOpName(fcmpType) << " float "
         << getRValue(0)->toString() << ", " << getRValue(1)->toString();
}
// %conv1 = fptosi float %add to i32
void FptosiInst::printIR(ostream& stream) const {
  Value* src = getRValue(0);
  stream << toString() << " = fptosi " << src->getType()->toString() << " "
         << src->toString() << " to i32";
}

// %arrayidx = getelementptr inbounds [5 x i32], ptr %arr, i32 0, i32 1
void GetElemPtrInst::printIR(ostream& stream) const {
  stream << toString() << " = getelementptr inbounds "
         << ((PointerType*)ptrType)->getElemType()->toString();
  int argSize = getRValueSize();
  for (int i = 0; i < argSize; i++) {
    Value* arg = getRValue(i);
    stream << ", " << arg->getType()->toString() << " " << arg->toString();
  }
}

void JumpInst::printIR(ostream& stream) const {
  stream << "br label " << getRValue(0)->toString();
}

// %0 = load i32, ptr %a.addr
void LoadInst::printIR(ostream& stream) const {
  stream << toString() << " = load " << getType()->toString() << ", "
         << getRValue(0)->getType()->toString() << " "
         << getRValue(0)->toString();
}

// %y = phi i32 [ %y.0, %if.then ], [ %neg, %if.else ]
void PhiInst::printIR(ostream& stream) const {
  int icSize = getRValueSize() / 2;
  assert(icSize);
  stream << toString() << " = phi " << getType()->toString();
  for (int i = 0; i < icSize; i++) {
    if (i != 0) {
      stream << ", ";
    }
    stream << "[ " << getRValue(i * 2)->toString() << ", "
           << getRValue(i * 2 + 1)->toString() << " ]";
  }
}

// ret i32 %conv1
void ReturnInst::printIR(ostream& stream) const {
  stream << "ret ";
  if (getRValueSize() == 0) {
    stream << "void";
  } else {
    Value* rValue = getRValue(0);
    stream << rValue->getType()->toString() << " " << rValue->toString();
  }
}

// %conv = sitofp i32 %1 to float
void SitofpInst::printIR(ostream& stream) const {
  Value* src = getRValue(0);
  stream << toString() << " = sitofp " << src->getType()->toString() << " "
         << src->toString() << " to float";
}

// store i32 %a, ptr %a.addr
void StoreInst::printIR(ostream& stream) const {
  Value* src = getRValue(0);
  Value* dst = getRValue(1);
  stream << "store " << src->getType()->toString() << " " << src->toString()
         << ", " << dst->getType()->toString() << " " << dst->toString();
}

// %lnot.ext = zext i1 %lnot to i32
void ZextInst::printIR(ostream& stream) const {
  Value* src = getRValue(0);
  stream << toString() << " = zext " << src->getType()->toString() << " "
         << src->toString() << " to " << getType()->toString();
}

/*********************** Clone Function ******************************/

Instruction* AllocaInst::clone() {
  AllocaInst* newInstr = new AllocaInst(getElemType(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* BinaryOpInst::clone() {
  BinaryOpInst* newInstr = new BinaryOpInst(getOpTag(), getType(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* BranchInst::clone() {
  BranchInst* newInstr = new BranchInst();
  newInstr->setParent(block);
  return newInstr;
}

Instruction* CallInst::clone() {
  CallInst* newInstr = new CallInst(getFunction(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* IcmpInst::clone() {
  IcmpInst* newInstr = new IcmpInst(getOpTag(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* FcmpInst::clone() {
  FcmpInst* newInstr = new FcmpInst(getOpTag(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* FptosiInst::clone() {
  FptosiInst* newInstr = new FptosiInst(getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* GetElemPtrInst::clone() {
  GetElemPtrInst* newInstr =
      new GetElemPtrInst(getType(), (PointerType*)getPtrType(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* JumpInst::clone() {
  JumpInst* newInstr = new JumpInst();
  newInstr->setParent(block);
  return newInstr;
}

Instruction* LoadInst::clone() {
  LoadInst* newInstr = new LoadInst(getType(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* PhiInst::clone() {
  PhiInst* newInstr = new PhiInst(getType(), getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* ReturnInst::clone() {
  ReturnInst* newInstr = new ReturnInst();
  newInstr->setParent(block);
  return newInstr;
}

Instruction* SitofpInst::clone() {
  SitofpInst* newInstr = new SitofpInst(getName());
  newInstr->setParent(block);
  return newInstr;
}

Instruction* StoreInst::clone() {
  StoreInst* newInstr = new StoreInst();
  newInstr->setParent(block);
  return newInstr;
}

Instruction* ZextInst::clone() {
  ZextInst* newInstr = new ZextInst(getType(), getName());
  newInstr->setParent(block);
  return newInstr;
}