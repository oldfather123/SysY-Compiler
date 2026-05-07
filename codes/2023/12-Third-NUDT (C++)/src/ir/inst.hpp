#ifndef __INST_HPP__
#define __INST_HPP__

#include <bits/stdc++.h>

#include "core.hpp"
#include "error.hpp"
#include "util.hpp"

namespace IR {

class RetInst : public Instruction {
public:
  RetInst(Value *ret = nullptr);
  Value *ret();
};

class JmpInst : public Instruction {
public:
  JmpInst(BasicBlock *bb);
  BasicBlock *bb();
};

class BrInst : public Instruction {
public:
  BrInst(Value *cond, BasicBlock *then_target, BasicBlock *else_target);
  BasicBlock *iftrue();
  BasicBlock *iffalse();
};

class UnaryInst : public Instruction {
public:
  UnaryInst(IType itype, Type *type, const string &name, Value *rhs);
  Value *rhs();
};

class BinaryInst : public Instruction {
public:
  BinaryInst(IType itype, Type *type, const string &name, Value *lhs, Value *rhs);
  Value *lhs();
  Value *rhs();
};

class GlobalInst : public Instruction {
public:
  GlobalInst(const string &name, Type *type, const vector<Value *> &inits);
  auto inits();
};

class AllocaInst : public Instruction {
public:
  AllocaInst(const string &name, Type *type);
};

class LoadInst : public Instruction {
public:
  LoadInst(const string &name, Value *addr);
  Value *addr();
};

class StoreInst : public Instruction {
public:
  StoreInst(Value *val, Value *addr);
  Value *val();
  Value *addr();
};

class GetelementptrInst : public Instruction {
public:
  GetelementptrInst(const string &name, Value *addr, std::vector<Value *> indexs);
  Value *addr();
  Value *index();
};

class IcmpInst : public Instruction {
public:
  IcmpInst(IType itype, const string &name, Value *lhs, Value *rhs);
  Value *lhs();
  Value *rhs();
};

class FcmpInst : public Instruction {
public:
  FcmpInst(IType itype, const string &name, Value *lhs, Value *rhs);
  Value *lhs();
  Value *rhs();
};

class PhiInst : public Instruction {
protected:
  size_t size;

public:
  PhiInst(Type *type, const string &name, const vector<Value *> &vals, const vector<BasicBlock *> &bbs);
  auto vals() {
    return Util::range(_operands.begin(), _operands.begin() + size);
  }
  auto bbs() {
    return Util::range(_operands.begin() + size, _operands.begin() + 2 * size);
  }
};

class CallInst : public Instruction {
public:
  CallInst(const string &name, Function *callee, const vector<Value *> &params);
  Function *callee();
  auto params();
};

}  // namespace IR

#endif
