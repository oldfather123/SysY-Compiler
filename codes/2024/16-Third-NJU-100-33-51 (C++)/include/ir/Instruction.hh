/**
 * Instruction ir
 * Create by Zhang Junbin at 2024/6/1
 */

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <unordered_map>

#include "LabelManager.hh"
#include "Value.hh"
using std::unordered_map;
class BasicBlock;
class Function;
class PointerType;

enum OpTag {
  ADD,
  FADD,
  SUB,
  FSUB,
  MUL,
  FMUL,
  SDIV,
  FDIV,
  SREM,
  FREM,
  LSHR,
  ASHR,
  SHL,
  AND,
  OR,
  XOR,
  EQ,
  NE,
  SLE,
  SLT,
  SGE,
  SGT,
  OEQ,
  ONE,
  OGT,
  OGE,
  OLE,
  OLT,
};

class Instruction : public Value {
 protected:
  unique_ptr<vector<Use*>> useList;
  BasicBlock* block = 0;

 public:
  Instruction(ValueTag vtag);
  Instruction(string name, ValueTag vTag);
  Instruction(Type* t, string name, ValueTag vTag);
  string getOpName(OpTag op) const;
  void pushValue(Value* v);
  Value* getRValue(int idx) const;
  bool deleteRValueAt(int idx);
  void swapRValueAt(int idx1, int idx2);
  void replaceRValueAt(int idx, Value* newValue);
  int getRValueSize() const { return useList->size(); }
  BasicBlock* getParent() { return block; }
  void setParent(BasicBlock* bb) { block = bb; }
  void eraseFromParent();
  // delete use operand
  void deleteUseList();
  // clone the instruction, not clone useList
  virtual Instruction* clone() = 0;
  void cloneUseList(unordered_map<Value*, Value*>& replaceMap,
                    vector<Use*>* fromUseList);
  vector<Use*>* getUseList() { return useList.get(); }
  void moveBefore(Instruction* instr);
  bool mayReadFromMemory();
  bool mayWriteToMemory();
};

class AllocaInst : public Instruction {
 private:
  Type* elemType;

 public:
  AllocaInst(Type* type, string name);
  void printIR(ostream& stream) const override;
  Type* getElemType() const { return elemType; }
  Instruction* clone() override;
};

class BinaryOpInst : public Instruction {
 private:
  OpTag bOpType;
  BinaryOpInst(OpTag opType, Type* type, string name)
      : Instruction(type, name, VT_BOP), bOpType(opType) {};

 public:
  BinaryOpInst(OpTag opType, Value* op1, Value* op2, string name);
  void printIR(ostream& stream) const override;
  const OpTag getOpTag() const { return bOpType; }
  Instruction* clone() override;
  void setOpTag(OpTag op) { bOpType = op; }
};

class BranchInst : public Instruction {
 private:
  BranchInst() : Instruction(VT_BR) {}

 public:
  BranchInst(Value* cond, BasicBlock* trueBlock, BasicBlock* falseBlock);
  bool replaceDestinationWith(BasicBlock* oldBlock, BasicBlock* newBlock);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class CallInst : public Instruction {
 private:
  Function* function;

 public:
  CallInst(Function* func, string name);
  CallInst(Function* func, vector<Value*>& params, string name);
  void pushArgument(Value* value);
  void printIR(ostream& stream) const override;
  Function* getFunction() const { return function; }
  void setFunction(Function* func) { function = func; }
  Instruction* clone() override;
};

class IcmpInst : public Instruction {
 private:
  OpTag icmpType;
  IcmpInst(OpTag opType, string name);

 public:
  IcmpInst(OpTag opType, Value* op1, Value* op2, string name);
  void printIR(ostream& stream) const override;
  const OpTag getOpTag() const { return icmpType; }
  void setOpTag(OpTag opTag) { icmpType = opTag; }
  Instruction* clone() override;
};

class FcmpInst : public Instruction {
 private:
  OpTag fcmpType;
  FcmpInst(OpTag opType, string name);

 public:
  FcmpInst(OpTag opType, Value* op1, Value* op2, string name);
  void printIR(ostream& stream) const override;
  const OpTag getOpTag() const { return fcmpType; }
  void setOpTag(OpTag opTag) { fcmpType = opTag; }
  Instruction* clone() override;
};

class FptosiInst : public Instruction {
  FptosiInst(string name);

 public:
  FptosiInst(Value* src, string name);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class GetElemPtrInst : public Instruction {
 private:
  Type* ptrType;
  GetElemPtrInst(Type* type, PointerType* ptrType_, string name);

 public:
  GetElemPtrInst(Value* ptr, Value* idx1, Value* idx2, string name);
  GetElemPtrInst(Value* ptr, Value* idx1, string name);
  void printIR(ostream& stream) const override;
  const Type* getPtrType() const { return ptrType; }
  Instruction* clone() override;
};

class JumpInst : public Instruction {
  JumpInst() : Instruction(VT_JUMP) {}

 public:
  JumpInst(BasicBlock* block);
  bool replaceDestinationWith(BasicBlock* oldBlock, BasicBlock* newBlock);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class LoadInst : public Instruction {
  LoadInst(Type* type, string name) : Instruction(type, name, VT_LOAD) {}

 public:
  LoadInst(Value* addr, string name);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class PhiInst : public Instruction {
 public:
  PhiInst(Type* type, string name) : Instruction(type, name, VT_PHI) {}
  PhiInst(string name);
  void printIR(ostream& stream) const override;
  void pushIncoming(Value* v, BasicBlock* bb);
  Value* deleteIncomingFrom(BasicBlock* block);
  Instruction* clone() override;
};

class ReturnInst : public Instruction {
 public:
  ReturnInst(Value* retValue);
  ReturnInst();
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class SitofpInst : public Instruction {
 private:
  SitofpInst(string name);

 public:
  SitofpInst(Value* src, string name);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class StoreInst : public Instruction {
  StoreInst() : Instruction(VT_STORE) {}

 public:
  StoreInst(Value* value, Value* addr);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

class ZextInst : public Instruction {
  ZextInst(Type* dstType, string name) : Instruction(dstType, name, VT_ZEXT) {}

 public:
  ZextInst(Value* src, Type* dstType, string name);
  void printIR(ostream& stream) const override;
  Instruction* clone() override;
};

#endif