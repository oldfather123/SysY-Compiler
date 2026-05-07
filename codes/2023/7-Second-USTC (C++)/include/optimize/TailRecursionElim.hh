#pragma once

#include "BasicBlock.hh"
#include "Function.hh"
#include "Instruction.hh"
#include "Module.hh"
#include "Pass.hh"


class TailRecursionElim : public Pass {
  public:
    TailRecursionElim(Module *m,bool print_ir=false) : Pass(m, print_ir) {}
    void execute() final;
    CallInst *get_candidate(BasicBlock *);
    void create_header();
    bool eliminate_call(CallInst *);

    const std::string get_name() const override {return name;}

  private:
    Function *f_;
    BasicBlock* preheader;
    BasicBlock* latch;
    BasicBlock* header;
    BasicBlock* return_bb;
    BasicBlock* cur_bb;
    std::vector<PhiInst *> phi_args, args_latch;
    Instruction *ret_phi;
    PhiInst* acc_phi;
    PhiInst* acc_latch;
    std::string name;
};