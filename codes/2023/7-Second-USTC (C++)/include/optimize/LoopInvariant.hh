#pragma once

#include "Module.hh"
#include "Pass.hh"
#include "CFGAnalyse.hh"
#include <vector>
#include <map>
#include <stack>
#include <set>
#include <memory>

class LoopInvariant : public Pass{
    
public:
    LoopInvariant(Module* m, bool print_ir=false): Pass(m, print_ir){}
    void execute() final;

    void find_invariants(std::vector<BasicBlock *>* loop);
    void move_invariants_out(std::vector<BasicBlock *>* loop);

    const std::string get_name() const override {return name;}
    

private:
    std::vector<std::pair<BasicBlock*, std::vector<Instruction*>>> invariants;
    std::unique_ptr<CFGAnalyse> CFG_analyser;
    std::string name = "LoopInvariant";
};







