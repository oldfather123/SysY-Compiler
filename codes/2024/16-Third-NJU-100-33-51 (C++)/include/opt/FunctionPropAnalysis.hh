/**
 * A pass to analysis some properties of functions
 */
#ifndef _FUNCTIONPROPANALYSIS_H_
#define _FUNCTIONPROPANALYSIS_H_
#include "Optimization.hh"

class FunctionPropAnalysis : public Optimization {
 public:
  FunctionPropAnalysis() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif