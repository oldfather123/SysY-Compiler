/**
 * GEP Simplify
 */
#ifndef _GEP_SIMPLIFY_H_
#define _GEP_SIMPLIFY_H_

#include "Optimization.hh"

class GEPSimplify : public Optimization {
 private:
  bool runOnBasicBlock(BasicBlock* block);

 public:
  GEPSimplify() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif