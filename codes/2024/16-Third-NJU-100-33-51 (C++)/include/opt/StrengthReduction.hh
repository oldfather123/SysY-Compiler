//
// Strength Reduction pass
//
#ifndef _STRENGTH_REDUCTION_H_
#define _STRENGTH_REDUCTION_H_

#include "Optimization.hh"

class StrengthReduction : public Optimization {
 public:
  StrengthReduction() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif