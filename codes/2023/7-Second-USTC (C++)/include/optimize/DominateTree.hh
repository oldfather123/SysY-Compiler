#pragma once

#include <set>

#include "BasicBlock.hh"
#include "Pass.hh"
#include "Module.hh"
#include "logging.hh"



class DominateTree : public Pass
{
public:
    explicit DominateTree(Module* module, bool print_ir=false): Pass(module, print_ir){}
    void execute()final;

    void get_post_order(BasicBlock *bb, std::set<BasicBlock* >& visited);
    void get_reverse_post_order(Function * func);
    void get_df(Function *func);        //& set dominance frontier
    void get_idom(Function *func);      //& build idom tree
    BasicBlock* intersect(BasicBlock* finger1, BasicBlock* finger2);
    void debug_print_bb2int();
    void debug_print_reverse_post_order();
    void debug_print_df(Function * func);
    const std::string get_name() const override {return name;}
    std::list<BasicBlock* > reverse_post_order; //& save BasicBlock in reverse postorder
private:
    std::map<BasicBlock*, int> bb2int;  //& map BasicBlock to an int with postorder (bb2int[root] = bb2int.size()-1)
    std::vector<BasicBlock* > doms;
    std::string name = "DominateTree";
};