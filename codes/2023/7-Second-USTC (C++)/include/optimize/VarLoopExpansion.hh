#pragma once 

#include "Pass.hh"
#include "CFGAnalyse.hh"
#include "Module.hh"
#include <memory>

class VarLoopExpansion : public Pass
{
private:
    Module *module_;
    std::unique_ptr<CFGAnalyse> CFG_analyser;
    std::string name = "VarLoopExpansion";
public:
    explicit VarLoopExpansion(Module* module, bool print_ir=false): Pass(module, print_ir){module_=module;}
    void execute() final;
    void find_try();
    void unrool_var_loop(std::vector<BasicBlock*>* loop);
    int const_val_check(Instruction *inst);
    const std::string get_name() const override {return name;}
};