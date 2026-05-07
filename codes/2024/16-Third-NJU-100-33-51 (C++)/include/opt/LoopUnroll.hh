/**
 * Loop Unroll
 */
#ifndef _LOOP_UNROLL_H_
#define _LOOP_UNROLL_H_

#include "LoopInfo.hh"
#include "Optimization.hh"

#define UNROLL_FACTOR 12  // Unroll time
#define MAX_LINE 20

struct ClonedLoop {
  LoopInfo* originLoop;
  unordered_map<Value*, Value*> valueMap;

  ClonedLoop(LoopInfo* originLoop_) : originLoop(originLoop_) {}
  BasicBlock* getHeader() {
    return (BasicBlock*)valueMap.at(originLoop->header);
  }
  Value* getNewValue(Value* oldValue) {
    auto it = valueMap.find(oldValue);
    return it != valueMap.end() ? it->second : nullptr;
  }
};

class LoopUnroll : public Optimization {
 private:
  bool canAllUnroll(LoopInfo* loopInfo);
  bool allUnroll(LoopInfo* loopInfo);
  ClonedLoop* cloneLoop(LoopInfo* originLoop);
  bool runOnLoop(LoopInfo* loopInfo);
  uint32_t collectBlocksAndCountLine(LoopInfo* loopInfo,
                                     unordered_set<BasicBlock*>& blocks);
  bool constantOrInvariant(Value* value, LoopInfo* loopInfo);

 public:
  LoopUnroll() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif