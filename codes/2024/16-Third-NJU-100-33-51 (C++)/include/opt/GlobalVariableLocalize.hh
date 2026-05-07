//
// GlobalVariable to LocalVariable
//
#ifndef _GLOBALVARIABLE_TO_LOCALVARIABLE_H_
#define _GLOBALVARIABLE_TO_LOCALVARIABLE_H_

#include "Optimization.hh"
class GlobalVariableLocalize : public Optimization {
 private:
  unordered_set<Function*> changedFunctions;
  bool localize(GlobalVariable* gv);

 public:
  GlobalVariableLocalize() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif