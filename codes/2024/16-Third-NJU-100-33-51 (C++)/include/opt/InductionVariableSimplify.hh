/**
 * Induction Variable Simplify
 */
#ifndef _INDUCTION_VARIABLE_SIMPLIFY_H_
#define _INDUCTION_VARIABLE_SIMPLIFY_H_

#include "LoopInfo.hh"
#include "Optimization.hh"
class InductionVariableSimplify : public Optimization {
 private:
  bool isInvariant(LoopInfo* loopInfo, Value* value);
  bool runOnLoop(LoopInfo* loopInfo);

 public:
  InductionVariableSimplify() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

struct IndVar {
  PhiInst* phiInst;
  BinaryOpInst* strideInst;
  Value* strideValue;
  Value* initValue;
  BinaryOpInst* remBop;
};

#endif