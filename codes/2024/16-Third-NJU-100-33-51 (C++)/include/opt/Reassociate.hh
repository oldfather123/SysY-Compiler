/**
 * Reassociate
 */
#ifndef _REASSOCIATE_H_
#define _REASSOCIATE_H_

#include "Optimization.hh"

class Reassociate : public Optimization {
 private:
  bool runOnBasicBlock(BasicBlock* block);
  bool canReduce(BinaryOpInst* instr, Value* arg);

 public:
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif