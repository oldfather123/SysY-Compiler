#ifndef _ALIASANALYSIS_H_
#define _ALIASANALYSIS_H_
#include "Optimization.hh"
class AliasAnalysis : public Optimization {
 public:
  AliasAnalysis() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif