#ifndef __PASS_RENAME_HPP__
#define __PASS_RENAME_HPP__

#include "core.hpp"

namespace Pass {

class Rename : public FunctionPass {
public:
  std::string name() override {
    return "rename";
  }
  void run(IR::Function* pass_unit) override {
    std::set<IR::Value*> rename_progress;
    pass_unit->resetNames();
    std::function<void(IR::Value*)> rename = [&](IR::Value* value) -> void {
      if (!value->type()->isVoid() && value->name()[0] == '%')
        value->rename(pass_unit->newName(value->name().substr(0, 1)));
      rename_progress.insert(value);
    };
    for (const auto& param : pass_unit->params())
      if (!rename_progress.count(param.get())) rename(param.get());
    for (const auto& bb : pass_unit->basicBlocks()) {
      if (!rename_progress.count(bb.get())) rename(bb.get());
      for (const auto& inst : bb->instructions())
        if (!rename_progress.count(inst.get())) rename(inst.get());
    }
  }
};

}  // namespace Pass

#endif
