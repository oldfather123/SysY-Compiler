#ifndef __PASS_CORE_HPP__
#define __PASS_CORE_HPP__

#include "ir.hpp"

namespace Pass {

template <typename PassUnit>
class Pass {
public:
  virtual ~Pass() {
  }
  virtual void run(PassUnit* pass_unit) = 0;
  virtual std::string name() = 0;
};

using CompileUnitPass = Pass<IR::CompileUnit>;
using FunctionPass = Pass<IR::Function>;
using BasicBlockPass = Pass<IR::BasicBlock>;

template <typename PassUnit>
class PassMana : public Pass<PassUnit> {
protected:
  std::vector<Pass<PassUnit>*> _passes;

public:
  virtual ~PassMana() {
  }
  virtual std::string name() override {
    return "PassMana";
  }
  virtual void run(PassUnit* pass_unit) override {
    for (const auto& pass : _passes) pass->run(pass_unit);
  }
  std::vector<Pass<PassUnit>*>& passes() {
    return _passes;
  }
  void newPass(Pass<PassUnit>* pass) {
    _passes.emplace_back(pass);
  }
};

using CompileUnitPassMana = PassMana<IR::CompileUnit>;
using FunctionPassMana = PassMana<IR::Function>;
using BasicBlockPassMana = PassMana<IR::BasicBlock>;

extern CompileUnitPassMana compile_unit_pass_mana;
extern FunctionPassMana function_pass_mana;
extern BasicBlockPassMana basic_block_pass_mana;

}  // namespace Pass

#endif
