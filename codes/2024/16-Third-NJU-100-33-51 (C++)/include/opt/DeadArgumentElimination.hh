/**
 * Dead Argument Elimination
 */
#ifndef _DEAD_ARGUMENT_ELIMINATION_H_
#define _DEAD_ARGUMENT_ELIMINATION_H_

#include "Optimization.hh"

class DeadArgumentElimination : public Optimization {
 public:
  DeadArgumentElimination() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif