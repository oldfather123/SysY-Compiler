#pragma once

#include"Module.hh"
#include"Pass.hh"
#include<set>


class ActiveVar : public Pass{
public:
    ActiveVar(Module *m, bool print_ir=false) : Pass(m, print_ir) {}
    ~ActiveVar() {}
    void execute() final;
    void get_def_use();
    void get_in_out_var();
    const std::string get_name() const override {return name;}

private:
    Function* func;
    std::map<BasicBlock *, std::set<Value *>> map_use, map_def;
    std::map<BasicBlock *, std::set<Value *>> map_phi;
    std::map<BasicBlock *, std::set<Value *>> flive_in, flive_out;
    std::map<BasicBlock *, std::set<Value *>> ilive_in, ilive_out;
    const std::string name = "ActiveVar";
};
