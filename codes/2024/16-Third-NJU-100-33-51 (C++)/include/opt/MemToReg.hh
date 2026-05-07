#ifndef _MEMToREG_H_
#define _MEMToREG_H_
#include <map>

#include "Function.hh"
#include "Optimization.hh"

using std::map;

struct ValueInfo;
class MemToReg : public Optimization {
 private:
  Function* function = 0;
  map<Instruction*, ValueInfo*> instToValueInfo;
  LinkedList<Instruction*> trashList;
  LinkedList<ValueInfo*> valueInfos;
  void runPass();
  bool linkDefsAndUsesToVar(ValueInfo* valueInfo);
  void renameRecursive(BasicBlock* bb);

 public:
  MemToReg() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif