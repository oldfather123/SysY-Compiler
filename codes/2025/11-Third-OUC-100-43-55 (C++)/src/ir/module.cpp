#include "module.h"

namespace ir {

void Module::addGlobal(std::unique_ptr<GlobalVariable> global) {
  _globals.emplace(global->getRawName(), std::move(global));
}

void Module::addFunction(std::unique_ptr<Function> function) {
  _functions.emplace(function->getRawName(), std::move(function));
}

void Module::addMFunction(std::unique_ptr<FuncBase> mFunc) {
  _mFuncs.push_back(std::move(mFunc));
}

bool Module::hasGlobal() const { return !_globals.empty(); }

bool Module::hasFunction() const { return !_functions.empty(); }

GlobalVariable *Module::getGlobal(const std::string &name) {
  auto it = _globals.find(name);
  return it != _globals.end() ? it->second.get() : nullptr;
}

Function *Module::getFunction(const std::string &name) {
  auto it = _functions.find(name);
  return it != _functions.end() ? it->second.get() : nullptr;
}

FuncBase *Module::getMFunction(size_t index) {
  assert(index < _mFuncs.size() && "Index out of bounds");
  return _mFuncs.at(index).get();
}

std::vector<GlobalVariable *> Module::getGlobals() const {
  std::vector<GlobalVariable *> result;
  result.reserve(_globals.size());
  for (const auto &[name, ptr] : _globals) {
    result.push_back(ptr.get());
  }
  return result;
}

std::vector<Function *> Module::getFunctions() const {
  std::vector<Function *> result;
  result.reserve(_functions.size());
  for (const auto &[name, ptr] : _functions) {
    result.push_back(ptr.get());
  }
  return result;
}

std::vector<riscv::MachineFunc *> Module::getMFuncs() const {
  std::vector<riscv::MachineFunc *> result;
  result.reserve(_mFuncs.size());
  for (const auto &ptr : _mFuncs) {
    auto func = static_cast<riscv::MachineFunc *>(ptr.get());
    result.push_back(func);
  }
  return result;
}

} // namespace ir
