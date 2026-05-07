/**
 * Tail Recursion Elimination
 */
#ifndef _TAIL_RECURSION_ELIMINATION_H_
#define _TAIL_RECURSION_ELIMINATION_H_

#include "Optimization.hh"
#include "LabelManager.hh"

// This pass needs to be placed after loop optimization pass 
// because it make new loop
class TailRecursionElimination : public Optimization {
 private:
  bool findTailRecursionBlocks(Function* function,
                               vector<CallInst*>& tailRecurCalls);

 public:
  TailRecursionElimination() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif