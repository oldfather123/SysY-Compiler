/**
 * CFG Simplify
 */
#ifndef _CFG_SIMPLIFY_H_
#define _CFG_SIMPLIFY_H_

#include "Optimization.hh"

class CFGSimplify : public Optimization {
 public:
  CFGSimplify() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif