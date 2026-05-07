/**
 * Global Value Numbering
 */
#ifndef _GLOBAL_VALUE_NUMBERING_H_
#define _GLOBAL_VALUE_NUMBERING_H_
#include <functional>

#include "Optimization.hh"

class GlobalValueNumbering : public Optimization {
 private:
  string hashToString(Instruction* instr);
  bool canNumbering(Instruction* instr);

 public:
  GlobalValueNumbering() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif