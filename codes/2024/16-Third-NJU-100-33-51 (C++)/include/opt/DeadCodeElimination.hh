/**
 * Dead code elimination
 */
#ifndef _DEAD_CODE_ELIMINATION_H_
#define _DEAD_CODE_ELIMINATION_H_
#include "Optimization.hh"

class DeadCodeElimination : public Optimization {
 private:
  bool isComputeInstruction(Instruction* instr);
  bool eliminateDeadBlocks(Function* func);
  bool eliminateDeadInstructions(Function* func);
  bool simplifyInstruction(Function* func);

 public:
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif