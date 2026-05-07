#ifndef _LOOPINVARIANTCODEMOTION_H_
#define _LOOPINVARIANTCODEMOTION_H_

#include <functional>
#include <unordered_set>

#include "AliasAnalysisResult.hh"
#include "LinkedList.hh"
#include "LoopInfo.hh"
#include "Optimization.hh"

using std::unordered_set;

class LoopInvariantCodeMotion : public Optimization {
 private:
  bool isInvariant(Value* instr, LoopInfo* loopInfo,
                   unordered_set<Instruction*>& invarients);
  bool isMovable(Instruction* instr, LoopInfo* LoopInfo,
                 unordered_set<Instruction*>& invarients,
                 AliasAnalysisResult* aaResult);
  bool runOnOneLoop(LoopInfo* loopInfo, AliasAnalysisResult* aaResult);

 public:
  LoopInvariantCodeMotion() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif