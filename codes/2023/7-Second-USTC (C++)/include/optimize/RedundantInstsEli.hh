#pragma once

#include <vector>

#include "Pass.hh"
#include "Instruction.hh"

class RedundantInstsEli : public Pass{
private:
    Module *module_;
    std::string name = "RedundantInstsEli";
    BasicBlock *cur_bb;

public:
    explicit RedundantInstsEli(Module *module, bool print_ir=false) : Pass(module, print_ir) { module_ = module; }
    void execute() final;
    const std::string get_name() const override {return name;}
    ~RedundantInstsEli(){};

    void remove_same_phi();
    void replace_phi();
    void remove_redundant_geps();
};