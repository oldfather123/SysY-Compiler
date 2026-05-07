//
// Load Elimination
//
// If a load follows a store and they operate on the same memory location, with
// no other load in between, then the load can be replaced with the value of the
// store
//
#ifndef _LOAD_ELIMINATION_H_
#define _LOAD_ELIMINATION_H_
#include "Optimization.hh"
class LoadElimination : public Optimization {
 private:
  bool runOnBasicBlock(BasicBlock* bb, AliasAnalysisResult* aaResult);

 public:
  LoadElimination() {}
  bool runOnModule(ANTPIE::Module* module);
  bool runOnFunction(Function* func);
};

#endif