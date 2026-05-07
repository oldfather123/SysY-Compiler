/**
 * IR module
 * Create by Zhang Junbin at 2024/6/2
 */
#ifndef _MODULE_H_
#define _MODULE_H_

#include "Function.hh"
#include "GlobalVariable.hh"

namespace ANTPIE {
class Module {
 private:
  LinkedList<GlobalVariable*> globalVariables;
  unordered_set<Function*> externFunctions;
  LinkedList<Function*> functions;
  BasicBlock* currBasicBlock;
  unordered_map<string, Function*> libFunctions;

 public:
  Module();
  ~Module();
  void pushFunction(Function* function);
  void pushGlobalVariable(GlobalVariable* globalVariable);
  void pushExternFunction(Function* function);
  void printIR(ostream& stream) const;
  void setCurrBasicBlock(BasicBlock* bb) { currBasicBlock = bb; }
  void pushInstrution(Instruction* instr) { currBasicBlock->pushInstr(instr); }
  Function* addFunction(FuncType* funcType, string name);
  BasicBlock* addBasicBlock(Function* function, string name);

  AllocaInst* addAllocaInst(Type* type, string name);
  BinaryOpInst* addBinaryOpInst(OpTag opType, Value* op1, Value* op2,
                                string name);
  BranchInst* addBranchInst(Value* cond, BasicBlock* trueBlock,
                            BasicBlock* falseBlock);
  CallInst* addCallInst(Function* func, string name);
  CallInst* addCallInst(Function* func, vector<Value*>& params, string name);
  IcmpInst* addIcmpInst(OpTag opType, Value* op1, Value* op2, string name);
  FcmpInst* addFcmpInst(OpTag opType, Value* op1, Value* op2, string name);
  FptosiInst* addFptosiInst(Value* src, string name);
  GetElemPtrInst* addGetElemPtrInst(Value* ptr, Value* idx1, Value* idx2,
                                    string name);
  GetElemPtrInst* addGetElemPtrInst(Value* ptr, Value* idx1, string name);
  JumpInst* addJumpInst(BasicBlock* block);
  LoadInst* addLoadInst(Value* addr, string name);
  PhiInst* addPhiInst(string name);
  ReturnInst* addReturnInst(Value* retValue);
  ReturnInst* addReturnInst();
  SitofpInst* addSitofpInst(Value* src, string name);
  StoreInst* addStoreInst(Value* value, Value* addr);
  ZextInst* addZextInst(Value* src, Type* dstType, string name);

  GlobalVariable* addGlobalVariable(Type* type, string name);
  GlobalVariable* addGlobalVariable(Type* type, Constant* init, string name);

  LinkedList<GlobalVariable*>* getGlobalVariables() { return &globalVariables; }
  LinkedList<Function*>* getFunctions() { return &functions; }
  unordered_set<Function*>* getexternFunctions() { return &externFunctions; }

  void addLibFunction(Function* func) { libFunctions[func->getName()] = func; }
  Function* getLibFunction(string name) { return libFunctions[name]; }
  
  void buildCFG();

  void irOptimize();
};
}  // namespace ANTPIE

#endif