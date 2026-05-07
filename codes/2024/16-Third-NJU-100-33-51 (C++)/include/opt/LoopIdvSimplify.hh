#ifndef _LOOP_IDV_SIMPLIFY_H_
#define _LOOP_IDV_SIMPLIFY_H_

#include "LoopInfo.hh"
#include "Optimization.hh"

class LoopIdvSimplify : public Optimization {
 private:
  bool runOnLoop(LoopInfo* loopInfo);

 public:
  LoopIdvSimplify() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif