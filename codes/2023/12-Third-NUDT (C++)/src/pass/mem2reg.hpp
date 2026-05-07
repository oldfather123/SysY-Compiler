#ifndef __PASS_MEM2REG_HPP__
#define __PASS_MEM2REG_HPP__

#include "core.hpp"
class Mem2Reg : public Pass::FunctionPass {
public:
  std::string name() override {
    return "mem2reg";
  }
  void run(IR::Function* pass_unit) override {
    return;
  }
};

#endif
