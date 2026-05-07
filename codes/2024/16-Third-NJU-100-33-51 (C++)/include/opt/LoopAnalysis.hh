#ifndef _LOOP_ANALYSIS_H_
#define _LOOP_ANALYSIS_H_

#include "LoopInfo.hh"
#include "Optimization.hh"

class LoopAnalysis : public Optimization {
 private:
  void discoverAndMapSubloop(LoopInfo* loopInfo, vector<BasicBlock*>& latches,
                             LoopInfoBase* li, DomTree* dt, CFG* cfg);

 public:
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif