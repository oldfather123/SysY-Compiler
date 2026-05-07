#ifndef __IRBUILDER_HPP__
#define __IRBUILDER_HPP__

#include "inst.hpp"

namespace IR {

using namespace IR;

class Builder {
private:
  CompileUnit *_prev_cu;
  Function *_prev_f;
  BasicBlock *_prev_bb;
  Instructions::iterator _prev_ptr;
  CompileUnit *_cu;
  Function *_f;
  BasicBlock *_bb;
  Instructions::iterator _ptr;

public:
  Builder();
  Builder(CompileUnit *cu);
  void initialize(CompileUnit *cu);
  void saveState();
  void loadState();
  void setState(CompileUnit *cu, Function *f, BasicBlock *bb, Instructions::iterator ptr);
  const string newName();
  CompileUnit *cu();
  Function *f();
  BasicBlock *bb();
  Instructions::iterator ptr();
  /// @brief Terminator Instructions
  RetInst *newRetInst(Value *ret);
  JmpInst *newJmpInst(BasicBlock *target);
  BrInst *newBrInst(Value *cond, BasicBlock *iftrue, BasicBlock *iffalse);
  /// @brief Unary Operations
  UnaryInst *newUnaryInst(IType itype, Type *type, const string &name, Value *rhs);
  UnaryInst *newSUnaryInst(IType itype, Type *type, const string &name, Value *rhs);
  UnaryInst *newFUnaryInst(IType itype, Type *type, const string &name, Value *rhs);
  UnaryInst *newFnegInst(const string &name, Value *rhs);
  /// @brief Binary Operations
  BinaryInst *newBinaryInst(IType itype, Type *type, const string &name, Value *lhs, Value *rhs);
  BinaryInst *newAddInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newFaddInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newSubInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newFsubInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newMulInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newFmulInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newSdivInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newFdivInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newSremInst(const string &name, Value *lhs, Value *rhs);
  /// @brief Bitwise Binary Operations
  BinaryInst *newShlInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newLshrInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newAshrInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newAndInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newOrInst(const string &name, Value *lhs, Value *rhs);
  BinaryInst *newXorInst(const string &name, Value *lhs, Value *rhs);
  /// @brief Memory Access and Addressing Operations
  GlobalInst *newGlobalInst(const string &name, Type *type, const vector<Value *> &inits);
  AllocaInst *newAllocaInst(const string &name, Type *type);
  LoadInst *newLoadInst(const string &name, Value *addr);
  StoreInst *newStoreInst(Value *val, Value *addr);
  GetelementptrInst *newGetelementptrInst(const string &name, Value *addr, std::vector<Value *> indexs);
  /// @brief Conversion Operations
  UnaryInst *newTruncInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newZextInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newSextInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newFptruncInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newFpextInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newFptosiInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newSitofpInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newPtrtointInst(const string &name, Value *rhs, Type *type);
  UnaryInst *newInttoptrInst(const string &name, Value *rhs, Type *type);
  /// @brief Other Operations
  IcmpInst *newIcmpInst(IType itype, const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIeqInst(const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIneInst(const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIsgtInst(const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIsgeInst(const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIsltInst(const string &name, Value *lhs, Value *rhs);
  IcmpInst *newIsleInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFcmpInst(IType itype, const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFoeqInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFoneInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFogtInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFogeInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFoltInst(const string &name, Value *lhs, Value *rhs);
  FcmpInst *newFoleInst(const string &name, Value *lhs, Value *rhs);
  PhiInst *newPhiInst(Type *type, const string &name, const vector<Value *> &vals, const vector<BasicBlock *> &bbs);
  CallInst *newCallInst(const string &name, Function *callee, const vector<Value *> &params);
};

}  // namespace IR

#endif
