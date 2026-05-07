#pragma once

#include "IR.h"
#include "riscv.h"
#include <memory.h>
#include <string.h>
#include <cassert>

const extern std::map<Instruction::OpID, RiscvInstr::InstrType> toRiscvOp;

extern int LabelCount;
extern std::map<BasicBlock *, RiscvBasicBlock *> rbbLabel;
extern std::map<::Function *, RiscvFunction *> functionLabel;

RiscvBasicBlock *createRiscvBasicBlock(BasicBlock *bb = nullptr);
RiscvFunction *createRiscvFunction(::Function *foo = nullptr);
std::string toLabel(int ind);
int calcTypeSize(IRType *ty);

class IRtoRISCV
{
public:
  IRtoRISCV()
  {
    rm = new RiscvModule();
  }
  RiscvModule *rm;
  int computeConstantBinaryResult(Instruction::OpID opId, int lhs, int rhs);
  std::string buildRISCV(Module *m);
  BinaryRiscvInst *createBinaryInstr(RegAlloca *regAlloca,BinaryInst *binaryInstr,RiscvBasicBlock *rbb);
  UnaryRiscvInst *createUnaryInstr(RegAlloca *regAlloca, UnaryInst *unaryInstr,RiscvBasicBlock *rbb);
  std::vector<RiscvInstr *> createStoreInstr(RegAlloca *regAlloca,StoreInst *storeInstr,RiscvBasicBlock *rbb);
  std::vector<RiscvInstr *> createLoadInstr(RegAlloca *regAlloca,LoadInst *loadInstr,RiscvBasicBlock *rbb);
  ICmpRiscvInstr *createICMPInstr(RegAlloca *regAlloca, ICmpInst *icmpInstr,BranchInst *brInstr, RiscvBasicBlock *rbb);
  ICmpRiscvInstr *createICMPSInstr(RegAlloca *regAlloca, ICmpInst *icmpInstr,RiscvBasicBlock *rbb);
  RiscvInstr *createFCMPInstr(RegAlloca *regAlloca, FCmpInst *fcmpInstr,RiscvBasicBlock *rbb);
  SiToFpRiscvInstr *createSiToFpInstr(RegAlloca *regAlloca,SiToFpInst *sitofpInstr,RiscvBasicBlock *rbb);
  FpToSiRiscvInstr *createFptoSiInstr(RegAlloca *regAlloca,FpToSiInst *fptosiInstr,RiscvBasicBlock *rbb);
  CallRiscvInst *createCallInstr(RegAlloca *regAlloca, CallInst *callInstr,RiscvBasicBlock *rbb);
  RiscvBasicBlock *transferRiscvBasicBlock(BasicBlock *bb, RiscvFunction *foo);
  ReturnRiscvInst *createRetInstr(RegAlloca *regAlloca, ReturnInst *returnInstr,RiscvBasicBlock *rbb, RiscvFunction *rfoo);
  BranchRiscvInstr *createBrInstr(RegAlloca *regAlloca, BranchInst *brInstr,RiscvBasicBlock *rbb);
  RiscvInstr *solveGetElementPtr(RegAlloca *regAlloca, GetElementPtrInst *instr,RiscvBasicBlock *rbb);
  void initRetInstr(RegAlloca *regAlloca, RiscvInstr *returnInstr,RiscvBasicBlock *rbb, RiscvFunction *foo);
};
