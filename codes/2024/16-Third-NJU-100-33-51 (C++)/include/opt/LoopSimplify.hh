#ifndef _LOOPSIMPLIFY_H_
#define _LOOPSIMPLIFY_H_

#include "LoopInfo.hh"
#include "Optimization.hh"

class LoopSimplify : public Optimization {
 private:
  bool needGuard = 0;
  bool simplyOneLoop(LoopInfo* loopInfo, CFG* cfg);
  bool simplyOneLoopWithGuard(LoopInfo* loopInfo, CFG* cfg);

 public:
  LoopSimplify(bool ng = false) { needGuard = ng; }
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif