#ifndef IR_INSTRUCTIONS_H
#define IR_INSTRUCTIONS_H

#include "instruction.h"
#include <string>

namespace ir {

class Function;

//===---------------- Binary ----------------===//

enum class BinaryOp { ADD, SUB, MUL, SDIV, SREM, XOR, FADD, FSUB, FMUL, FDIV };

class BinaryInst : public Instruction {
private:
  BinaryOp _op;
  static std::string_view opToString(BinaryOp op);

public:
  BinaryInst(BinaryOp op, Value *lhs, Value *rhs);
  BinaryInst(std::unique_ptr<Type> type, BinaryOp op)
      : Instruction(std::move(type)) {}

  BinaryOp getOp() const;
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

//===---------------- Cmp ----------------===//

enum class CmpOp {
  EQ, // Integer
  NE,
  SLT,
  SGT,
  SLE,
  SGE,
  OEQ, // Float
  OLT,
  OGT,
  OLE,
  OGE,
  UNE,
};

class CmpInst : public Instruction {
private:
  CmpOp _op;
  static std::string_view opToString(CmpOp op);

public:
  CmpInst(CmpOp op, Value *lhs, Value *rhs);

  bool isICmpInst() const;
  CmpOp getOp() const;
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

//===---------------- Cast ----------------===//

enum class CastOp { BitCast, FPToSI, SIToFP, SExt, ZExt };

class CastInst : public Instruction {
private:
  CastOp _op;
  static std::string_view opToString(CastOp op);

public:
  CastInst(std::unique_ptr<Type> targetType, CastOp op, Value *val);

  CastOp getCastOp() const;
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

//===---------------- Terminator ----------------===//

class RetInst : public Instruction {
public:
  RetInst(Value *retVal);
  RetInst();
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
  bool isTerminator() const override;
};

class BranchInst : public Instruction {
public:
  BranchInst(Value *cond, BasicBlock *trueBlock, BasicBlock *falseBlock);
  BranchInst(BasicBlock *target); // unconditional branch

  bool isConditional() const;
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
  bool isTerminator() const override;
};

//===---------------- Other Instructions ----------------===//

class AllocaInst : public Instruction {
public:
  AllocaInst(std::unique_ptr<Type> allocType);
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

class LoadInst : public Instruction {
public:
  using Instruction::Instruction;
  LoadInst(Value *ptr);
  LoadInst(std::unique_ptr<Type> loadedType, Value *ptr);
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

class StoreInst : public Instruction {
public:
  StoreInst(Value *val, Value *ptr);
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

class GetElementPtrInst : public Instruction {
private:
  static std::unique_ptr<Type> calcType(Value *value, size_t indexSize);

public:
  GetElementPtrInst(Value *base, const std::vector<Value *> &indices);
  GetElementPtrInst(std::unique_ptr<Type> targetType);
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
};

class CallInst : public Instruction {
public:
  CallInst(Function *func, const std::vector<Value *> &args);
  InstKind getInstKind() const override;
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override;
  BasicKind retTypeKind() const {
    return static_cast<BasicType *>(getType())->getBasicKind();
  }
};

//===---------------- Fake Instructions ----------------===//

class PhiInst : public Instruction {
private:
  std::vector<BasicBlock *> _incoming;

public:
  PhiInst(std::unique_ptr<Type> type);

  InstKind getInstKind() const override { return InstKind::Phi; }
  void addIncoming(Value *value, BasicBlock *block);
  BasicBlock *getIncomingBlock(size_t index) const;
  void setIncomingBlock(size_t index, BasicBlock *block);
  void removeIncoming(size_t index);
  std::string str() const override;
  std::unique_ptr<Instruction> cloneEmpty() const override {
    // TODO
    return nullptr;
  }
};

// Used to eliminate Phi Inst
class MoveInst : public Instruction {
private:
  PhiInst *_target;

public:
  MoveInst(PhiInst *target, Value *src);
  InstKind getInstKind() const override { return InstKind::Move; }
  PhiInst *getTarget() const { return _target; }
  // TODO
  std::string str() const override {

    return "move " + getName() + "\ttarget: " + getOperand(0)->str() +
           "\tsrc: " + getOperand(1)->getName();
  }
  std::unique_ptr<Instruction> cloneEmpty() const override {
    // TODO
    return nullptr;
  }
};

} // namespace ir

#endif // IR_INSTRUCTIONS_H
