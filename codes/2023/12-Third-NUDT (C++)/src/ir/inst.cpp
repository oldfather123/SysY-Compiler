#include "inst.hpp"

using namespace IR;

/// @brief Terminator Instructions
RetInst::RetInst(Value *ret) : Instruction(RET, Type::void_t(), "") {
  newOperand(ret);
}
Value *RetInst::ret() {
  return operand(0);
}

JmpInst::JmpInst(BasicBlock *dest) : Instruction(JMP, Type::void_t(), "") {
  newOperand(dest);
}
BasicBlock *JmpInst::bb() {
  return static_cast<BasicBlock *>(operand(0));
}

BrInst::BrInst(Value *cond, BasicBlock *iftrue, BasicBlock *iffalse) : Instruction(BR, Type::void_t(), "") {
  newOperand(cond);
  newOperand(iftrue);
  newOperand(iffalse);
}
BasicBlock *BrInst::iftrue() {
  return static_cast<BasicBlock *>(operand(1));
}
BasicBlock *BrInst::iffalse() {
  return static_cast<BasicBlock *>(operand(2));
}

/// @brief Unary Operations
UnaryInst::UnaryInst(IType itype, Type *type, const string &name, Value *rhs) : Instruction(itype, type, name) {
  newOperand(rhs);
}
Value *UnaryInst::rhs() {
  return operand(0);
}

/// @brief Binary Operations
BinaryInst::BinaryInst(IType itype, Type *type, const string &name, Value *lhs, Value *rhs)
    : Instruction(itype, type, name) {
  newOperand(lhs);
  newOperand(rhs);
}
Value *BinaryInst::lhs() {
  return operand(0);
}
Value *BinaryInst::rhs() {
  return operand(1);
}

/// @brief Memory Access and Addressing Operations
AllocaInst::AllocaInst(const string &name, Type *type) : Instruction(ALLOCA, PointerType::get(type), name) {
}

LoadInst::LoadInst(const string &name, Value *addr) : Instruction(LOAD, addr->type()->deref(1), name) {
  newOperand(addr);
}
Value *LoadInst::addr() {
  return operand(0);
}

StoreInst::StoreInst(Value *val, Value *addr) : Instruction(STORE, Type::void_t(), "") {
  newOperand(val);
  newOperand(addr);
}
Value *StoreInst::val() {
  return operand(0);
}
Value *StoreInst::addr() {
  return operand(1);
}

GetelementptrInst::GetelementptrInst(const string &name, Value *addr, std::vector<Value *> indexs)
    : Instruction(GETELEMENTPTR, Type::pointer_t(addr->type()->deref(indexs.size())), name) {
  newOperand(addr);
  newOperands(indexs);
}
Value *GetelementptrInst::addr() {
  return operand(0);
}
Value *GetelementptrInst::index() {
  return operand(1);
}

/// @brief Other Operations
IcmpInst::IcmpInst(IType itype, const string &name, Value *lhs, Value *rhs) : Instruction(itype, Type::i1_t(), name) {
  newOperand(lhs);
  newOperand(rhs);
}
Value *IcmpInst::lhs() {
  return operand(0);
}
Value *IcmpInst::rhs() {
  return operand(1);
}

FcmpInst::FcmpInst(IType itype, const string &name, Value *lhs, Value *rhs) : Instruction(itype, Type::i1_t(), name) {
  newOperand(lhs);
  newOperand(rhs);
}
Value *FcmpInst::lhs() {
  return operand(0);
}
Value *FcmpInst::rhs() {
  return operand(1);
}

PhiInst::PhiInst(Type *type, const string &name, const vector<Value *> &vals, const vector<BasicBlock *> &bbs)
    : Instruction(PHI, type, name), size(vals.size()) {
  newOperands(vals);
  newOperands(bbs);
}

CallInst::CallInst(const string &name, Function *callee, const vector<Value *> &params)
    : Instruction(CALL, callee->retType(), name) {
  newOperand(callee);
  newOperands(params);
}
Function *CallInst::callee() {
  return static_cast<Function *>(operand(0));
}
auto CallInst::params() {
  return Util::range(_operands.begin() + 1, _operands.end());
}

GlobalInst::GlobalInst(const string &name, Type *type, const vector<Value *> &inits)
    : Instruction(GLOBAL, PointerType::get(type), name) {
  newOperands(inits);
}
auto GlobalInst::inits() {
  return Util::range(_operands.begin(), _operands.end());
}
