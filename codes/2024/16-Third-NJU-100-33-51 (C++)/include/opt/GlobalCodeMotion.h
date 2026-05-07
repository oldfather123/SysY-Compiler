/**
 * Global Code Motion
 */
#ifndef _GLOBAL_CODE_MOTION_H_
#define _GLOBAL_CODE_MOTION_H_


#include "DomTree.hh"
#include "Optimization.hh"
#include "LoopInfo.hh"

class GlobalCodeMotion : public Optimization {
 private:
  struct GCMInfo {
    BasicBlock* earliestBlock;
    BasicBlock* latestBlock;
    BasicBlock* bestBlock;
    GCMInfo() : earliestBlock(0), latestBlock(0), bestBlock(0) {}
  };
  unordered_set<Instruction*> visited;
  unordered_map<Instruction*, GCMInfo*> infoMap;

  bool isPinned(Instruction* instr);
  Instruction* scheduleEarly(Instruction* instr, DomTree* dt);
  Instruction* scheduleLate(Instruction* instr, DomTree* dt,
                            LoopInfoBase* liBase);

 public:
  GlobalCodeMotion() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif