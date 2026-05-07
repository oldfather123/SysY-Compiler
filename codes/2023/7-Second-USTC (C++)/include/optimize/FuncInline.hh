#pragma once
#include "Pass.hh"

class FuncInline : public Pass
{
private:
    /* data */
    std::vector<std::pair<Function*, std::pair<Instruction *,Function *>>> calling_pair;
    std::string name = "FuncInline";
    Module *module_;
    int MAX_INSTRUCTION_NUM = 100;
public:
    explicit FuncInline(Module *module, bool print_ir=false) : Pass(module, print_ir) { module_ = module;}
    void execute() override;
    const std::string get_name() const override {return name;}
    void inline_call_find();
    void func_inline();
};

