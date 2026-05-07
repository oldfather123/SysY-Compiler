/**
 * Merge basicblock
 */
#ifndef _MERGEBLOCK_H_
#define _MERGEBLOCK_H_
#include "Optimization.hh"

class MergeBlock : public Optimization {
 public:
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif