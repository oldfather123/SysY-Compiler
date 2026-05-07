#ifndef IR_MODULE_H
#define IR_MODULE_H

#include "function.h"
#include "global_variable.h"
#include "machine_func.h"
#include <unordered_map>
#include <vector>

namespace ir {

class Module {
private:
  std::unordered_map<std::string, std::unique_ptr<GlobalVariable>> _globals;
  std::unordered_map<std::string, std::unique_ptr<Function>> _functions;
  std::vector<std::unique_ptr<FuncBase>> _mFuncs;

public:
  Module() = default;

  void addGlobal(std::unique_ptr<GlobalVariable> global);
  void addFunction(std::unique_ptr<Function> function);
  void addMFunction(std::unique_ptr<FuncBase> mfunc);

  bool hasGlobal() const;
  bool hasFunction() const;

  GlobalVariable *getGlobal(const std::string &name);
  Function *getFunction(const std::string &name);
  FuncBase *getMFunction(size_t index);

  size_t numGlobals() const { return _globals.size(); }
  size_t numFunctions() const { return _functions.size(); }
  size_t numMFuncs() const { return _mFuncs.size(); }

  // Return a viewport instead of the actual ownership
  std::vector<GlobalVariable *> getGlobals() const;
  // Return a viewport instead of the actual ownership
  std::vector<Function *> getFunctions() const;
  // Return a viewport instead of the actual ownership
  std::vector<riscv::MachineFunc *> getMFuncs() const;
};

} // namespace ir

#endif // IR_MODULE_H
