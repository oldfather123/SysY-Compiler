/**
 * Constant folding
 */
#ifndef _CONSTANT_FOLDING_H_
#define _CONSTANT_FOLDING_H_

#include <cmath>

#include "Optimization.hh"
class ConstantFolding : public Optimization {
 private:
  Value* instructionSimplify(Instruction* instr);
  bool constantFoldingDFS(BasicBlock* bb, DomTree* dt);
  bool constantOperand(Instruction* instr);
  Value* simplifyBOP(BinaryOpInst* instr);
  Value* simplifyICMP(IcmpInst* instr);
  Value* simplifyFCMP(FcmpInst* instr);
  Value* simplifySITOFP(SitofpInst* instr);
  Value* simplifyFPTOSI(FptosiInst* instr);
  Value* simplifyZEXT(ZextInst* instr);

 public:
  ConstantFolding(){};
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif