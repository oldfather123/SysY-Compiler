#include "builder.hpp"

using namespace IR;

Builder::Builder()
    : _prev_cu(nullptr),
      _prev_f(nullptr),
      _prev_bb(nullptr),
      _prev_ptr(nullptr),
      _cu(nullptr),
      _f(nullptr),
      _bb(nullptr),
      _ptr(nullptr) {
}
Builder::Builder(CompileUnit *cu)
    : _prev_cu(nullptr), _prev_f(nullptr), _prev_bb(nullptr), _prev_ptr(nullptr), _cu(cu) {
  _bb = _f->basicBlock(0);
  _ptr = _bb->instructions().end();
}
void Builder::initialize(CompileUnit *cu) {
  _cu = cu;
  _f = _cu->initFunction();
  _bb = _f->basicBlock(0);
  _ptr = _bb->instructions().end();
}
void Builder::saveState() {
  _prev_cu = _cu;
  _prev_f = _f;
  _prev_bb = _bb;
  _prev_ptr = _ptr;
}
void Builder::loadState() {
  _cu = _prev_cu;
  _f = _prev_f;
  _bb = _prev_bb;
  _ptr = _prev_ptr;
}
void Builder::setState(CompileUnit *cu, Function *f, BasicBlock *bb, Instructions::iterator ptr) {
  _cu = cu;
  _f = f;
  _bb = bb;
  _ptr = ptr;
}
const string Builder::newName() {
  return f()->newName(cu()->isGlobalScope() ? "@" : "%");
}
CompileUnit *Builder::cu() {
  return _cu;
}
Function *Builder::f() {
  return _f;
}
BasicBlock *Builder::bb() {
  return _bb;
}
Instructions::iterator Builder::ptr() {
  return _ptr;
}

/// @brief Terminator Instructions
RetInst *Builder::newRetInst(Value *ret) {
  assert(ret->type()->in({Type::void_t(), Type::i32_t(), Type::float_t()}));
  auto *instruction = new RetInst(ret);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
JmpInst *Builder::newJmpInst(BasicBlock *target) {
  assert(target);
  auto *instruction = new JmpInst(target);
  assert(instruction);
  bb()->succs().emplace_back(target);
  target->preds().emplace_back(bb());
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
BrInst *Builder::newBrInst(Value *cond, BasicBlock *iftrue, BasicBlock *iffalse) {
  assert(cond->type()->isI1());
  assert(iftrue && iffalse);
  auto *instruction = new BrInst(cond, iftrue, iffalse);
  assert(instruction);
  bb()->succs().emplace_back(iftrue);
  iftrue->preds().emplace_back(bb());
  bb()->succs().emplace_back(iffalse);
  iffalse->preds().emplace_back(bb());
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}

/// @brief Unary Operations
UnaryInst *Builder::newUnaryInst(IType itype, Type *type, const string &name, Value *rhs) {
  auto *instruction = new UnaryInst(itype, type, name, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
UnaryInst *Builder::newSUnaryInst(IType itype, Type *type, const string &name, Value *rhs) {
  assert(rhs->type()->in({Type::i32_t()}));
  return newUnaryInst(itype, type, name, rhs);
}
UnaryInst *Builder::newFUnaryInst(IType itype, Type *type, const string &name, Value *rhs) {
  assert(rhs->type()->in({Type::float_t()}));
  return newUnaryInst(itype, type, name, rhs);
}
UnaryInst *Builder::newFnegInst(const string &name, Value *rhs) {
  return newFUnaryInst(FNEG, Type::float_t(), name, rhs);
}

/// @brief Binary Operations
BinaryInst *Builder::newBinaryInst(IType itype, Type *type, const string &name, Value *lhs, Value *rhs) {
  assert(lhs->type() == type);
  assert(rhs->type() == type);
  auto *instruction = new BinaryInst(itype, type, name, lhs, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
BinaryInst *Builder::newAddInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(ADD, Type::i32_t(), name, lhs, rhs);
}
BinaryInst *Builder::newFaddInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(FADD, Type::float_t(), name, lhs, rhs);
}
BinaryInst *Builder::newSubInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(SUB, Type::i32_t(), name, lhs, rhs);
}
BinaryInst *Builder::newFsubInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(FADD, Type::float_t(), name, lhs, rhs);
}
BinaryInst *Builder::newMulInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(MUL, Type::i32_t(), name, lhs, rhs);
}
BinaryInst *Builder::newFmulInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(FADD, Type::float_t(), name, lhs, rhs);
}
BinaryInst *Builder::newSdivInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(SDIV, Type::i32_t(), name, lhs, rhs);
}
BinaryInst *Builder::newFdivInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(FDIV, Type::float_t(), name, lhs, rhs);
}
BinaryInst *Builder::newSremInst(const string &name, Value *lhs, Value *rhs) {
  return newBinaryInst(SREM, Type::i32_t(), name, lhs, rhs);
}

/// @brief Bitwise Binary Operations
BinaryInst *Builder::newShlInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}
BinaryInst *Builder::newLshrInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}
BinaryInst *Builder::newAshrInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}
BinaryInst *Builder::newAndInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}
BinaryInst *Builder::newOrInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}
BinaryInst *Builder::newXorInst(const string &name, Value *lhs, Value *rhs) {
  throw Unreachable();
}

/// @brief Memory Access and Addressing Operations
GlobalInst *Builder::newGlobalInst(const string &name, Type *type, const vector<Value *> &inits) {
  assert(type);
  auto *instruction = new GlobalInst(name, type, inits);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
AllocaInst *Builder::newAllocaInst(const string &name, Type *type) {
  assert(type);
  auto *instruction = new AllocaInst(name, type);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
LoadInst *Builder::newLoadInst(const string &name, Value *addr) {
  assert(addr->type()->isPointer());
  auto *instruction = new LoadInst(name, addr);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
StoreInst *Builder::newStoreInst(Value *val, Value *addr) {
  assert(addr->type()->isPointer());
  assert(addr->type()->deref(1) == val->type());
  auto *instruction = new StoreInst(val, addr);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
GetelementptrInst *Builder::newGetelementptrInst(const string &name, Value *addr, std::vector<Value *> indexs) {
  assert(addr->type()->isPointer() || addr->type()->isArray());
  assert(indexs.size());
  auto *instruction = new GetelementptrInst(name, addr, indexs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}

/// @brief Conversion Operations
UnaryInst *Builder::newTruncInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}
UnaryInst *Builder::newZextInst(const string &name, Value *rhs, Type *type) {
  assert(rhs->type()->sizeOf() < type->sizeOf());
  auto *instruction = new UnaryInst(ZEXT, type, name, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
UnaryInst *Builder::newSextInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}
UnaryInst *Builder::newFptruncInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}
UnaryInst *Builder::newFpextInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}
UnaryInst *Builder::newFptosiInst(const string &name, Value *rhs, Type *type) {
  assert(rhs->type()->in({Type::float_t()}));
  assert(type->in({Type::i32_t()}));
  auto *instruction = new UnaryInst(SITOFP, type, name, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
UnaryInst *Builder::newSitofpInst(const string &name, Value *rhs, Type *type) {
  assert(rhs->type()->in({Type::i32_t()}));
  assert(type->in({Type::float_t()}));
  auto *instruction = new UnaryInst(SITOFP, type, name, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
UnaryInst *Builder::newPtrtointInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}
UnaryInst *Builder::newInttoptrInst(const string &name, Value *rhs, Type *type) {
  throw Unreachable();
}

/// @brief Other Operations
IcmpInst *Builder::newIcmpInst(IType itype, const string &name, Value *lhs, Value *rhs) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type()->in({Type::i1_t(), Type::i32_t()}) || lhs->type()->isPointer());
  assert(rhs->type()->in({Type::i1_t(), Type::i32_t()}) || rhs->type()->isPointer());
  auto *instruction = new IcmpInst(itype, name, lhs, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
IcmpInst *Builder::newIeqInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(IEQ, name, lhs, rhs);
}
IcmpInst *Builder::newIneInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(INE, name, lhs, rhs);
}
IcmpInst *Builder::newIsgtInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(ISGT, name, lhs, rhs);
}
IcmpInst *Builder::newIsgeInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(ISGE, name, lhs, rhs);
}
IcmpInst *Builder::newIsltInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(ISLT, name, lhs, rhs);
}
IcmpInst *Builder::newIsleInst(const string &name, Value *lhs, Value *rhs) {
  return newIcmpInst(ISLE, name, lhs, rhs);
}
FcmpInst *Builder::newFcmpInst(IType itype, const string &name, Value *lhs, Value *rhs) {
  assert(lhs->type() == rhs->type());
  assert(lhs->type()->in({Type::float_t()}));
  assert(rhs->type()->in({Type::float_t()}));
  auto *instruction = new FcmpInst(itype, name, lhs, rhs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
FcmpInst *Builder::newFoeqInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FOEQ, name, lhs, rhs);
}
FcmpInst *Builder::newFoneInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FONE, name, lhs, rhs);
}
FcmpInst *Builder::newFogtInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FOGT, name, lhs, rhs);
}
FcmpInst *Builder::newFogeInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FOGE, name, lhs, rhs);
}
FcmpInst *Builder::newFoltInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FOLT, name, lhs, rhs);
}
FcmpInst *Builder::newFoleInst(const string &name, Value *lhs, Value *rhs) {
  return newFcmpInst(FOLE, name, lhs, rhs);
}
PhiInst *Builder::newPhiInst(Type *type, const string &name, const vector<Value *> &vals,
                             const vector<BasicBlock *> &bbs) {
  assert(vals.size() == bbs.size());
  for (const auto &val : vals)
    if (val->type() != type) throw InvalidInst();
  auto *instruction = new PhiInst(type, name, vals, bbs);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
CallInst *Builder::newCallInst(const string &name, Function *callee, const vector<Value *> &params) {
  assert(callee);
  auto *instruction = new CallInst(name, callee, params);
  assert(instruction);
  setState(cu(), f(), bb(), ++bb()->instructions().emplace(ptr(), instruction));
  return instruction;
}
